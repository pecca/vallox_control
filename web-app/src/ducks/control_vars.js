
import { fromJS } from 'immutable';
import _ from 'lodash';

import { readVars, writeVar } from './../services/vallox_ctrl_api';

const defaultState = null;

export function getControlVars() {
  return (dispatch, getState) => {
    readVars('control_vars').then(function(msg) {
      dispatch(controlVarsUpdate(msg.digit_vars));
    });
  }
}

export function setControlVar(handle, value) {
  return (dispatch, getState) => {
    writeVar("control_var", handle, value);
    dispatch(controlVarWriteReq(handle, value));
  }
}

export function controlVarsUpdate(controlVars) {
  return {
    type: 'CONTROL_VARS_UPDATE',
    payload: {
        controlVars,
    }
  }
}

export function controlVarWriteReq(handle, value) {
  return {
    type: 'CONTROL_VAR_WRITE_REQ',
    payload: {
      handle,
      value,
    }
  }
}

export default function (state = defaultState, action) {
  const { type, payload } = action;
    switch (type) {

      case 'CONTROL_VARS_UPDATE':
        if (state === null) {
          state = fromJS(payload.controlVars);
          _.forEach(payload.controlVars, (value, key) => {
            state = state.setIn([key, 'updateCnt'], 1);
            state = state.setIn([key, 'writeReq'], false);
          });
        }
        else {
          _.forEach(payload.controlVars, (value, key) => {
            if (value.ts > state.getIn([key, 'ts'])) {
              const updateCnt = state.getIn([key, 'updateCnt']);
              state = state.setIn([key, 'updateCnt'], updateCnt + 1);
              state = state.setIn([key, 'ts'], value.ts);
              state = state.setIn([key, 'value'], fromJS(value.value));
              state = state.setIn([key, 'writeReq'], false);
            }
          });
        }
        return state;
      case 'CONTROL_VAR_WRITE_REQ':
        const { handle } = payload
        state = state.setIn([handle, 'writeReq'], true);
        return state;

      default:
        return state;
    }
}
