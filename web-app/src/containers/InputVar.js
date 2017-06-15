
import React, { Component } from 'react'
import { connect } from 'react-redux';
import { bindActionCreators } from 'redux';
import { FormGroup, FormControl, Button, Form, Label, OverlayTrigger, Tooltip } from 'react-bootstrap'

import { setDigitVar } from './../ducks/digit_vars';

function tooltip(min, max) {
  return (
    <Tooltip id="tooltip">{`Value range: ${min}...${max}`}</Tooltip>
  );
}

class InputVar extends Component {

  state = { value: "" };

  handleOnChange(value) {
    const { item } = this.props;

    const num = Number(value);
    if (!isNaN(num) &&
        num >= item.min &&
        num <= item.max) {
      this.setState({value: num});
    }
    else if (value === '') {
      this.setState({value: value});
    }
  }

  handleSetButtonClick() {
    const { item, setDigitVar } = this.props;

    if (this.state.value !== '' &&
        item.value !== this.state.value) {
      this.setState({value: ""});
      setDigitVar(item.handle, this.state.value);
    }
  }

  render() {
    const { item } = this.props;
    if (item.writeReq) {
      return (
        <Label bsStyle="info"> Processing </Label>
      );
    }

    return (
      <Form inline>
        <FormGroup controlId="formInlineName">
          <OverlayTrigger placement="top" overlay={tooltip(item.min, item.max)}>
            <FormControl  bsSize="small"
                          style={{width:50, marginRight: 10}}
                          type="text"
                          value={this.state.value}
                          onChange={(event) => this.handleOnChange(event.target.value)}/>
          </OverlayTrigger>
          <Button bsSize="small"
                  disabled={(this.state.value === '')}
                  onClick={() => this.handleSetButtonClick()}>
            Set
          </Button>
        </FormGroup>
      </Form>
    );
  }
}

function mapStateToProps(state) {
  return {}
}

export default connect(mapStateToProps,
                       dispatch => bindActionCreators({
                         setDigitVar,
                       }, dispatch))(InputVar);
