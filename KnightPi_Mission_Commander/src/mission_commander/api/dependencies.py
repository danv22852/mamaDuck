from mission_commander.core.application_state import ApplicationState


_app_state = ApplicationState()


def get_app_state() -> ApplicationState:
    return _app_state
