
import React from 'react'
import { Panel, Table, Badge } from 'react-bootstrap'

function renderTable(readVars) {
  return (
    <Table>
      <thead>
        <tr>
          <th>Name</th>
          <th>Value</th>
          <th>Timestamp</th>
        </tr>
      </thead>
      <tbody>
      {
        readVars.map(item =>
          <tr key={item.name}>
            <td> {item.name}  </td>
            <td> {`${item.value} ${item.unit}`} </td>
            <td>
              {new Date(item.ts * 1000).toUTCString()} <Badge>{item.updateCnt}</Badge>
            </td>
          </tr>
        )
      }
      </tbody>
    </Table>
  );
}

function getReadFields(digitVars, handle, name, unit = '') {
  return {
    value: digitVars.getIn([handle, 'value']),
    ts: digitVars.getIn([handle, 'ts']),
    updateCnt: digitVars.getIn([handle, 'updateCnt']),
    name,
    unit
  }
}

const ReadVars = ({digitVars}) => {

  let vars = [];
  if (digitVars) {
    const degreeUnit = String.fromCharCode(8451);

    vars.push(getReadFields(digitVars, 'outside_temp', 'Outside temp', degreeUnit));
    vars.push(getReadFields(digitVars, 'exhaust_temp', 'Exhaust temp', degreeUnit));
    vars.push(getReadFields(digitVars, 'inside_temp', 'Inside temp', degreeUnit));
    vars.push(getReadFields(digitVars, 'incoming_temp', 'Incoming temp', degreeUnit));
    vars.push(getReadFields(digitVars, 'rh1_sensor', 'Humidity sensor', '%'));
    vars.push(getReadFields(digitVars, 'basic_rh_level', 'Humidity base level', '%'));
  }

  return (
    <Panel collapsible header="Read only variables">
      { renderTable(vars) }
    </Panel>
  );
};

export default ReadVars;
