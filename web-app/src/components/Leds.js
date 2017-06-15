
import React from 'react'
import _ from 'lodash'
import { Panel, Table, Badge} from 'react-bootstrap'
import Led from './Led'

function renderTable(leds)
{
  return (
    _.map(leds.value, (value, key) =>
      <tr key={key}>
        <td>{key}</td>
        <td>
          <Led status={value}/>
        </td>
      </tr>
    )
  );
}

const Leds = ({leds}) => {
  return (
    <Panel collapsible header="Panel leds">
      <Table>
        <thead>
          <tr>
            <th>Name</th>
            <th>Status</th>
          </tr>
        </thead>
        <tbody>
          { renderTable(leds) }
        </tbody>
      </Table>
      <div>
        <b>Timestamp:</b> {new Date(leds.ts * 1000).toUTCString()} <Badge> {leds.updateCnt} </Badge>
      </div>
    </Panel>
  );
};

export default Leds;
