import pytest

def pytest_addoption(parser):
    parser.addoption('--digitalis', action='store_true', help='connect to digitalis')
    parser.addoption('--digitalis2', action='store_true', help='connect to digitalis2')
    parser.addoption('--port', action='store', type=int, help='which port to use')
