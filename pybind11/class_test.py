import Class


class Test(Class.BaseInt):
    def foo(self):
        print("Test::foo()")
        
# 创建 DerivedInt 实例
test = Test()
test.foo()