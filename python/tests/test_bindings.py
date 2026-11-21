import gc
import importlib
import os
import subprocess
import sys
import sysconfig
import textwrap
import time
import unittest
import uuid
from pathlib import Path

import numpy as np


PYTHON_DIR = Path(__file__).resolve().parents[1]
BUILD_DIR = PYTHON_DIR / "build"
TEST_INSTALLED = os.environ.get("SHMSERVER_TEST_INSTALLED") == "1"
if not TEST_INSTALLED:
    sys.path.insert(0, str(BUILD_DIR / "generated"))
    sys.path.insert(0, str(BUILD_DIR))

import pack_message
import shm_connection
import shm_server
import spscqueue


class AbiSuffixTests(unittest.TestCase):
    def test_extension_modules_have_python_abi_suffix(self):
        ext_suffix = sysconfig.get_config_var("EXT_SUFFIX")
        self.assertTrue(ext_suffix)
        for module_name in (
            "_pack_message",
            "_shm_server",
            "_shm_connection",
            "_spscqueue",
        ):
            module = importlib.import_module(module_name)
            self.assertTrue(
                module.__file__.endswith(ext_suffix),
                f"{module.__file__} does not end with {ext_suffix}",
            )


class PackMessageTests(unittest.TestCase):
    def test_message_layout_and_strings(self):
        msg = pack_message.PackMessage()
        self.assertEqual(msg.to_bytes(), bytes(992))

        msg.MessageType = pack_message.EMessageType.EOrderRequest
        msg.OrderRequest.Colo = "上海"
        msg.OrderRequest.Account = "a" * 64
        msg.OrderRequest.ErrorMsg = b"no error"

        self.assertEqual(msg.MessageType, pack_message.EMessageType.EOrderRequest)
        self.assertEqual(msg.OrderRequest.Colo, "上海")
        self.assertEqual(msg.OrderRequest.Account, "a" * 15)
        self.assertEqual(msg.OrderRequest.ErrorMsg, "no error")
        self.assertIsInstance(msg.to_bytes(), bytes)

    def test_union_properties_and_enum_aliases(self):
        msg = pack_message.PackMessage()
        order = pack_message.TOrderStatus()
        order.Colo = "ZX"
        order.OrderRef = "reference"
        msg.OrderStatus = order

        self.assertEqual(msg.OrderStatus.Colo, "ZX")
        self.assertEqual(msg.OrderStatus.OrderRef, "reference")

        position = pack_message.TAccountPosition()
        position.BusinessType = pack_message.EBusinessType.EFUTURE
        position.FuturePosition.LongTdVolume = 12
        msg.AccountPosition = position
        self.assertEqual(msg.AccountPosition.FuturePosition.LongTdVolume, 12)

        self.assertEqual(int(pack_message.ESTOCK), 1)
        self.assertEqual(
            pack_message.EMessageType.EFutureMarketData.value,
            0xFFB1,
        )
        self.assertEqual(
            pack_message.EMsgType.EMSG_TYPE_HEARTBEAT.value,
            10,
        )

    def test_stock_market_data_arrays(self):
        data = pack_message.TStockMarketData()
        data.BidPrice = np.arange(10, dtype=np.float64)
        data.BidVolume = list(range(10, 20))
        data.AskPrice = np.arange(20, 30, dtype=np.float64)
        data.AskVolume = list(range(30, 40))

        self.assertEqual(data.BidPrice.dtype, np.float64)
        self.assertEqual(data.BidPrice.tolist(), list(map(float, range(10))))
        self.assertEqual(data.BidVolume.dtype, np.intc)
        self.assertEqual(data.BidVolume.tolist(), list(range(10, 20)))
        self.assertEqual(data.AskPrice.tolist(), list(map(float, range(20, 30))))
        self.assertEqual(data.AskVolume.tolist(), list(range(30, 40)))

        with self.assertRaisesRegex(RuntimeError, "size 10"):
            data.BidPrice = [1.0]


class CrossModuleTests(unittest.TestCase):
    def test_queue_and_prestart_connection(self):
        msg = pack_message.PackMessage()
        msg.ChannelID = 7
        msg.OrderRequest.Colo = "queue"

        queue = spscqueue.SPSCQueue()
        self.assertTrue(queue.Push(msg))

        received = pack_message.PackMessage()
        self.assertTrue(queue.Pop(received))
        self.assertEqual(received.ChannelID, 7)
        self.assertEqual(received.OrderRequest.Colo, "queue")

        client = shm_connection.SHMConnection("prestart-client")
        self.assertFalse(client.IsConnected())
        self.assertFalse(client.Push(msg))
        self.assertFalse(client.Pop(received))

        data_client = shm_connection.SHMDataConnection("prestart-data-client")
        self.assertFalse(data_client.IsConnected())
        self.assertFalse(data_client.Push(msg))
        self.assertFalse(data_client.Pop(received))

    def test_server_director_echo(self):
        server_name = f"swig_server_{os.getpid()}_{uuid.uuid4().hex[:8]}"
        client_name = f"swig_client_{uuid.uuid4().hex[:8]}"
        shm_path = Path("/dev/shm") / f"{server_name}.shm"

        server_code = textwrap.dedent(
            f"""
            import pack_message
            import shm_server

            class EchoServer(shm_server.SHMServer):
                def HandleMsg(self):
                    msg = pack_message.PackMessage()
                    if self.Pop(msg):
                        self.Push(msg)

            server = EchoServer()
            server.Start({server_name!r})
            while True:
                server.PollMsg()
                server.HandleMsg()
            """
        )

        env = os.environ.copy()
        if not TEST_INSTALLED:
            env["PYTHONPATH"] = os.pathsep.join(
                (str(BUILD_DIR), str(BUILD_DIR / "generated"))
            )
        server = subprocess.Popen(
            [sys.executable, "-u", "-c", server_code],
            env=env,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            text=True,
        )

        try:
            deadline = time.monotonic() + 10
            while not shm_path.exists():
                if server.poll() is not None:
                    stderr = server.stderr.read() if server.stderr else ""
                    self.fail(f"server exited before startup: {stderr}")
                if time.monotonic() > deadline:
                    self.fail("server shared memory was not created")
                time.sleep(0.02)

            client = shm_connection.SHMConnection(client_name)
            client.Start(server_name)
            self.assertTrue(client.IsConnected())

            payload = pack_message.PackMessage()
            payload.MessageType = pack_message.EMessageType.EOrderRequest
            payload.OrderRequest.Colo = "director"
            payload.OrderRequest.Ticker = "ag2501"
            self.assertTrue(client.Push(payload))

            response = pack_message.PackMessage()
            deadline = time.monotonic() + 5
            while time.monotonic() < deadline:
                if client.Pop(response):
                    break
                time.sleep(0.005)
            else:
                self.fail("director echo response was not received")

            self.assertEqual(response.OrderRequest.Colo, "director")
            self.assertEqual(response.OrderRequest.Ticker, "ag2501")
        finally:
            server.terminate()
            try:
                server.wait(timeout=3)
            except subprocess.TimeoutExpired:
                server.kill()
                server.wait(timeout=3)
            finally:
                if server.stderr:
                    server.stderr.close()
                gc.collect()
                try:
                    shm_path.unlink()
                except FileNotFoundError:
                    pass


if __name__ == "__main__":
    unittest.main(verbosity=2)
