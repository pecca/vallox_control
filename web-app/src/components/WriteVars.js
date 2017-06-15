
import React from 'react'
import { Panel, Table, Badge } from 'react-bootstrap'
import InputVar from './../containers/InputVar'

function renderTable(vars) {
  return (
    <Table>
      <thead>
        <tr>
          <th>Name</th>
          <th>Value</th>
          <th>Edit</th>
          <th>Timestamp</th>
        </tr>
      </thead>
      <tbody>
      {
        vars.map(item =>
          <tr key={item.handle}>

            <td> {item.name}  </td>
            <td> {`${item.value} ${item.unit}`} </td>
            <td style={{width:200}}>
              <InputVar item={item}/>
            </td>
            <td style={{width:300}}>
              {new Date(item.ts * 1000).toUTCString()} <Badge> {item.updateCnt}</Badge>
            </td>
          </tr>
        )
      }
      </tbody>
    </Table>
  );
}

function getWriteFields(digitVars, handle, name, min, max, unit = '') {
  return {
    handle,
    name,
    min,
    max,
    unit,
    value: digitVars.getIn([handle, 'value']),
    ts: digitVars.getIn([handle, 'ts']),
    writeReq: digitVars.getIn([handle, 'writeReq']),
    updateCnt: digitVars.getIn([handle, 'updateCnt']),
  }
}

const WriteVars = ({digitVars}) => {
  let vars = [];

  if (digitVars) {
    const degreeUnit = String.fromCharCode(8451);

    vars.push(getWriteFields(digitVars, 'cur_fan_speed', 'Current fan speed', 1, 8));
    vars.push(getWriteFields(digitVars, 'min_fan_speed', 'Minimum fan speed', 1, 8));
    vars.push(getWriteFields(digitVars, 'max_fan_speed', 'Maximum fan speed', 1, 8));
    vars.push(getWriteFields(digitVars, 'hrc_bypass_temp', 'Cell bypass temp', 14, 20, degreeUnit));
    vars.push(getWriteFields(digitVars, 'pre_heating_temp', 'Pre-heating temp', -3, 10, degreeUnit));
    vars.push(getWriteFields(digitVars, 'input_fan_stop_temp', 'Input fan stop temp', -3, 10, degreeUnit));
    vars.push(getWriteFields(digitVars, 'cell_defrosting_hysteresis', 'Cell defrost hysteris', 0, 3, degreeUnit));
    vars.push(getWriteFields(digitVars, 'dc_fan_input', 'Input fan power', 1, 100, '%'));
    vars.push(getWriteFields(digitVars, 'dc_fan_output', 'Output fan power', 1, 100, '%'));
  }
  return (
    <Panel collapsible header="Writable variables">
      {renderTable(vars)}
    </Panel>
  );
};

export default WriteVars;
