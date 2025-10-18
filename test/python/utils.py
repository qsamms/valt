import socket
from functools import wraps

def valt_test_class(HOST, PORT):
    def class_decorator(cls):
        def make_wrapper(attr):
            @wraps(attr)
            def wrapper(self, *args, **kwargs):
                with socket.create_connection((HOST, PORT)) as sock:
                    kwargs["sock"] = sock
                    return attr(self, *args, **kwargs)
                # socket auto-closes here
            return wrapper

        for name, attr in cls.__dict__.items():
            if callable(attr) and name.startswith("test"):
                setattr(cls, name, make_wrapper(attr))
        return cls
    return class_decorator
