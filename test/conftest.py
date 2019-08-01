import pytest


def pytest_addoption(parser):
    parser.addoption(
        "--include-slow",
        action="store_true",
        default=False,
        help="Run slow tests"
    )


def pytest_configure(config):
    config.addinivalue_line("markers", "slow: mark test as slow to run")


def pytest_collection_modifyitems(config, items):
    if config.getoption("--include-slow"):
        return
    skip_slow = pytest.mark.skip(reason="Use --include-slow to run this test")
    for item in items:
        if "slow" in item.keywords:
            item.add_marker(skip_slow)
