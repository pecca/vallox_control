import React from 'react';
import ReactDOM from 'react-dom';
import App from './containers/App';
import registerServiceWorker from './registerServiceWorker';
import './index.css';

import promiseMiddleware from 'redux-promise-middleware';
import thunk from 'redux-thunk';
import { createStore } from './utils/redux';
import * as reducers from './ducks';
import { browserHistory } from 'react-router';
import { Provider } from 'react-redux';


/*
if (__DEVELOPMENT__) {
  const Perf = require('react-addons-perf');
  window.Perf = Perf;
}
*/

let middleware = [
  thunk,
  promiseMiddleware(),
];

/*
if (__DEVELOPMENT__) {
  const createLogger = require('redux-logger');
  middleware = middleware.concat([createLogger()]);
}
*/

const { store } = createStore(
  reducers,
  browserHistory,
  middleware,
  [],
  undefined,
);

ReactDOM.render(
  <Provider store={store}>
    <App/>
  </Provider>,
  document.getElementById('root')
);

registerServiceWorker();
