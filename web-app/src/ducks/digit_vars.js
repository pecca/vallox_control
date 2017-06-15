
import { fromJS } from 'immutable';
import _ from 'lodash';

import { readDigitVars, writeDigitVar } from './../services/vallox_ctrl_api';

const defaultState = null;

export function getDigitVars() {
  return (dispatch, getState) => {
    readDigitVars().then(function(msg) {
      dispatch(digitVarsUpdate(msg.digit_vars));
    });
  }
}

export function setDigitVar(handle, value) {
  return (dispatch, getState) => {
    writeDigitVar(handle, value);
    dispatch(digitVarWriteReq(handle, value));
  }
}

export function digitVarsUpdate(digitVars) {
  return {
    type: 'DIGIT_VARS_UPDATE',
    payload: {
      digitVars,
    }
  }
}

export function digitVarWriteReq(handle, value) {
  return {
    type: 'DIGIT_VAR_WRITE_REQ',
    payload: {
      handle,
      value,
    }
  }
}

export default function (state = defaultState, action) {
  const { type, payload } = action;
    switch (type) {

      case 'DIGIT_VARS_UPDATE':
        if (state === null) {
          state = fromJS(payload.digitVars);
          _.forEach(payload.digitVars, (value, key) => {
            state = state.setIn([key, 'updateCnt'], 1);
            state = state.setIn([key, 'writeReq'], false);
          });
        }
        else {
          _.forEach(payload.digitVars, (value, key) => {
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
      case 'DIGIT_VAR_WRITE_REQ':
        const { handle } = payload
        state = state.setIn([handle, 'writeReq'], true);
        return state;

      default:
        return state;
    }
}
