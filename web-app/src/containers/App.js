
import React, {Component} from 'react';
import { PageHeader, PanelGroup, Button } from 'react-bootstrap';
import { connect } from 'react-redux';

import Leds from './../components/Leds';
import ReadVars from './../components/ReadVars';
import WriteVars from './../components/WriteVars';
import { getDigitVars } from './../ducks/digit_vars';

const LINK_TO_GRAFANA = 'http://178.217.128.239/#/dashboard/db/grafana';

class App extends Component {

  componentDidMount() {
    const { dispatch } = this.props;
    setInterval(() => dispatch(getDigitVars()), 3000);
  }

  render() {
    const { digitVars } = this.props;

    let leds = {};
    if (digitVars) {
      leds = digitVars.get('panel_leds').toJS();
    }
    return (
      <div>
        <PageHeader>
          Vallox 150 SE MLV <small> device and defrost control </small>
        </PageHeader>
        <PanelGroup>
          <Leds leds={leds}/>
          <ReadVars digitVars={digitVars}/>
          <WriteVars digitVars={digitVars}/>
        </PanelGroup>
        <Button href={LINK_TO_GRAFANA} bsStyle="link">Link to Grafana</Button>
      </div>
    );
  }
}

function mapStateToProps(state) {
  return {
    digitVars: state.digitVars,
  }
}

export default connect(mapStateToProps)(App);
