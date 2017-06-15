
import React from 'react'
import './Led.css'

const Led = ({status}) => {

  let styleClass;
  if (status) {
    styleClass = "led-on"
  }
  else {
    styleClass = "led-off"
  }

  return (
    <div className={styleClass}></div>
  );
}

export default Led;
