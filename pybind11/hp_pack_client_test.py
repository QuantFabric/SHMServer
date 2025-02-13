# coding: utf-8
import time
import sys
import os
import hp_pack_client

import shm_connection
import shm_server
# import pack_message


if __name__ == '__main__':
    
    cmd = sys.executable + " " + " ".join(sys.argv)
    print(cmd)

    client = hp_pack_client.HPPackClient(ip='192.168.1.168', port=8001)
    client.Start(account="SMAStrategy")
    client.UpdateAppStatus(cmd=cmd, app_name="hppack_client_test")
    while True:
        time.sleep(1)