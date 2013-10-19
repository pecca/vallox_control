<html>
<head>
<title> Vallox control </title>
</head>
<body>
<?php 

$page = $_SERVER['PHP_SELF'];
$sec = "30";
header("Refresh: $sec; url=$page");

define("CUR_FAN_SPEED", "29");
define("OUTSIDE_TEMP", "32");
define("EXHAUST_TEMP", "33");
define("INSIDE_TEMP", "34");
define("INCOMING_TEMP", "35");
define("POST_HEATING_ON_CNT", "55");
define("POST_HEATING_OFF_CNT", "56");
define("INCOMING_TARGET_TEMP", "A4");
define("PANEL_LEDS", "A3");
define("MAX_FAN_SPEED", "A5");
define("MIN_FAN_SPEED", "A9");
define("HRC_BYPASS_TEMP", "AF");
define("INPUT_FAN_STOP_TEMP", "A8"); 
define("CELL_DEFROSTING_HYSTERESIS", "B2");
define("DC_FAN_INPUT", "B0");
define("DC_FAN_OUTPUT", "B1");
define("FLAGS_2", "6D");
define("FLAGS_4", "6F");
define("FLAGS_5", "70");
define("FLAGS_6", "71");
define("RH1_SENSOR", "2F");
define("BASIC_RH_LEVEL", "AE");
define("PRE_HEATING_TEMP", "A7");

define("DS18B20_SENSOR1", "1");
define("DS18B20_SENSOR2", "2");
define("AM2302_TEMP", "1");
define("AM2302_RH", "2");

function get_digit_var($fp, $var_id, &$timestamp)
{
    $id = "DIGIT GET " . $var_id . " 0";
    fwrite($fp, $id);
    $str = fread($fp, 26);
    sscanf($str, "%s %d", $value, $timestamp);
    $timestamp = date('r', $timestamp);

    return $value;
}

function get_ds18b20_var($fp, $var_id, &$timestamp)
{
    $id = "DS18B20 GET " . $var_id . " 0";
    fwrite($fp, $id);
    $str = fread($fp, 26);
    sscanf($str, "%s %d", $value, $timestamp);
    $timestamp = date('r', $timestamp);

    return $value;
}

function get_am2302_var($fp, $var_id, &$timestamp)
{
    $id = "AM2302 GET " . $var_id . " 0";
    fwrite($fp, $id);
    $str = fread($fp, 26);
    sscanf($str, "%s %d", $value, $timestamp);
    $timestamp = date('r', $timestamp);

    return $value;
}

function set_digit_var($fp, $var_id, $value)
{
    $id = "DIGIT SET " . $var_id . " " . $value . " 0";
    fwrite($fp, $id);
}



$fp = fsockopen("udp://127.0.0.1", 32000, $errno, $errstr);

if (!$fp) {
    echo "ERROR: $errno - $errstr<br />\n";
}

	

    else if (isset($_POST['edit_cur_fan_speed'])) 
    {        
        set_digit_var($fp, CUR_FAN_SPEED, $_POST['edit_cur_fan_speed_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();

    }
    else if (isset($_POST['edit_min_fan_speed'])) 
    {        
        set_digit_var($fp, MIN_FAN_SPEED, $_POST['edit_min_fan_speed_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_dc_fan_input'])) 
    {        
        echo $_POST['edit_dc_fan_input_set'];
        set_digit_var($fp, DC_FAN_INPUT, $_POST['edit_dc_fan_input_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_dc_fan_output'])) 
    {        
        set_digit_var($fp, DC_FAN_OUTPUT, $_POST['edit_dc_fan_output_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_hrc_bypass_temp'])) 
    {    
        set_digit_var($fp, HRC_BYPASS_TEMP, $_POST['edit_hrc_bypass_temp_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_input_fan_stop_temp'])) 
    {        
        set_digit_var($fp, INPUT_FAN_STOP_TEMP, $_POST['edit_input_fan_stop_temp_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_cell_defrosting_hysteresis'])) 
    {        
        set_digit_var($fp, CELL_DEFROSTING_HYSTERESIS, $_POST['edit_cell_defrosting_hysteresis_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_incoming_target_temp'])) 
    {        
        set_digit_var($fp, INCOMING_TARGET_TEMP, $_POST['edit_incoming_target_temp_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_pre_heating_temp'])) 
    {        
        set_digit_var($fp, PRE_HEATING_TEMP, $_POST['edit_pre_heating_temp_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }


 








//$fp = fsockopen("udp://127.0.0.1", 32000, $errno, $errstr);

if (!$fp) {
    echo "ERROR: $errno - $errstr<br />\n";
}
else
{

    

    $cur_fan_speed = get_digit_var($fp, CUR_FAN_SPEED, $cur_fan_speed_ts);


    $outside_temp = get_digit_var($fp, OUTSIDE_TEMP, $outside_temp_ts);
    $exhaust_temp = get_digit_var($fp, EXHAUST_TEMP, $exhaust_temp_ts);
    
    $inside_temp = get_digit_var($fp, INSIDE_TEMP, $inside_temp_ts);
    $incoming_temp = get_digit_var($fp, INCOMING_TEMP, $incoming_temp_ts);

    $pre_heating_temp = get_digit_var($fp, PRE_HEATING_TEMP, $pre_heating_temp_ts);

    $post_heating_on_cnt = get_digit_var($fp, POST_HEATING_ON_CNT, $post_heating_on_cnt_ts);
    $post_heating_off_cnt = get_digit_var($fp, POST_HEATING_OFF_CNT, $post_heating_off_cnt_ts);
    $incoming_target_temp = get_digit_var($fp, INCOMING_TARGET_TEMP, $incoming_target_temp_ts);

    $panel_leds = get_digit_var($fp, PANEL_LEDS, $panel_leds_ts);
    


    $min_fan_speed = get_digit_var($fp, MIN_FAN_SPEED, $min_fan_speed_ts);
    // $max_fan_speed = get_digit_var($fp, MAX_FAN_SPEED, $max_fan_speed_ts);
    $hrc_bypass_temp = get_digit_var($fp, HRC_BYPASS_TEMP, $hrc_bypass_temp_ts);
    $input_fan_stop_temp = get_digit_var($fp, INPUT_FAN_STOP_TEMP, $input_fan_stop_temp_ts);
    $cell_defrosting_hysteresis = get_digit_var($fp, CELL_DEFROSTING_HYSTERESIS, $cell_defrosting_hysteresis_ts);
    $dc_fan_input = get_digit_var($fp, DC_FAN_INPUT, $dc_fan_input_ts);
    $dc_fan_output = get_digit_var($fp, DC_FAN_OUTPUT, $dc_fan_output_ts);
    $flags_2 = get_digit_var($fp, FLAGS_2, $flags_2_ts);
    $flags_4 = get_digit_var($fp, FLAGS_4, $flags_4_ts);
    $flags_5 = get_digit_var($fp, FLAGS_5, $flags_5_ts);
    $flags_6 = get_digit_var($fp, FLAGS_6, $flags_6_ts);
    $rh1_sensor = get_digit_var($fp, RH1_SENSOR, $rh1_sensor_ts);
    $basic_rh_level = get_digit_var($fp, BASIC_RH_LEVEL, $basic_rh_level_ts);

    $ds18b20_sensor1 = get_ds18b20_var($fp, DS18B20_SENSOR1, $ds18b20_sensor1_ts);
    $ds18b20_sensor2 = get_ds18b20_var($fp, DS18B20_SENSOR2, $ds18b20_sensor2_ts);
    
    $am2302_temp = get_am2302_var($fp, AM2302_TEMP, $am2302_temp_ts);
    $am2302_rh = get_am2302_var($fp, AM2302_RH, $am2302_rh_ts);

    $t = floatval($inside_temp);
    $rh = $rh1_sensor / 100; 
    $a = floatval(17.27);
    $b = floatval(237.7);

    $z =  ((($a * $t) / ($b + $t)) + log($rh));
    $dew_point = round((($b * $z) / ($a - $z)), 1);
    
  
    $air_flow = 15 + 10 * $cur_fan_speed;
    $radiator_watt = round($air_flow * 1.225 * ($ds18b20_sensor1 - $outside_temp),1); 
        

    $incoming_air_efficiency = round(floatval((floatval($incoming_temp) - floatval($ds18b20_sensor1)) / (floatval($inside_temp) - floatval($ds18b20_sensor1))) * 100,1);
    $outcoming_air_efficiency = round(floatval((floatval($inside_temp) - floatval($ds18b20_sensor2)) / (floatval($inside_temp) - floatval($ds18b20_sensor1))) * 100,1);

    fclose($fp);
}
?> 

<A HREF="/talo/">TaloLoggerGraph</A>

    <table border="1">
    <caption>Vallox 150 SE MLV: read variables</caption>
         <tr>
         <th>Variable</th>
         <th>Value</th>
         <th>Updated</th>
         </tr>
         <tr>
         <td>Outside temp</td>
         <td> <?php echo $outside_temp . " *C"; ?> </td>
         <td> <?php echo $outside_temp_ts; ?> </td>
         </tr>
         <tr> 
         <td>Exhaust temp</td>
         <td> <?php echo $exhaust_temp . " *C"; ?> </td>
         <td> <?php echo $exhaust_temp_ts; ?> </td>
         </tr>
         <tr>
         <td>Inside temp</td>
         <td> <?php echo $inside_temp . " *C"; ?> </td>
         <td> <?php echo $inside_temp_ts; ?> </td>
         </tr>
         <tr>
         <td>Incoming temp</td>
         <td> <?php echo $incoming_temp . " *C"; ?> </td>
         <td> <?php echo $incoming_temp_ts; ?> </td>
         </tr>
         <td>RH1 sensor</td>
         <td> <?php echo $rh1_sensor . " %"; ?> </td>
         <td> <?php echo $rh1_sensor_ts; ?> </td>
         </tr>
         <tr>
         <td>Basic RH level</td>
         <td> <?php echo $basic_rh_level . " %"; ?> </td>
         <td> <?php echo $basic_rh_level_ts; ?> </td>
         </tr>
         <tr>
         <td>Post heating ON counter</td>
         <td> <?php echo $post_heating_on_cnt . " sec"; ?> </td>
         <td> <?php echo $post_heating_on_cnt_ts; ?> </td>
         </tr>
         <tr>
         <td>Post heating OFF counter</td>
         <td> <?php echo $post_heating_off_cnt . " sec"; ?> </td>
         <td> <?php echo $post_heating_off_cnt_ts; ?> </td>
         </tr>
    </table>

    <table border="1">
    <caption>Vallox 150 SE MLV: edit variables</caption>
         <tr>
         <th>Variable</th>
         <th>Value</th>
         <th>Updated</th>
         <th>Set value</th>
         </tr>
         <tr>
         <td>CUR FAN speed</td>
         <td> <?php echo $cur_fan_speed; ?> </td>
         <td> <?php echo $cur_fan_speed_ts; ?> </td>
         <td>
         <form method="post">
         <input type="number" name="edit_cur_fan_speed_set" size="2" />
         <input type='submit' name='edit_cur_fan_speed' value="Set" />
         </form>
         </td>   
         </tr>
         <tr>
         <td>MIN FAN speed</td>
         <td> <?php echo $min_fan_speed; ?> </td>
         <td> <?php echo $min_fan_speed_ts; ?> </td>
         <td>
         <form method="post">
            <input type="number" name="edit_min_fan_speed_set" size="2" />
            <input type='submit' name='edit_min_fan_speed' value="Set" />
         </form>
         </td>   
         </tr>
         <tr>
         <td>DC fan input adj.</td>
         <td> <?php echo $dc_fan_input . " %";  ?> </td>
         <td> <?php echo $dc_fan_input_ts;  ?> </td>
         <td>
         <form method="post">
            <input type="text" name="edit_dc_fan_input_set" size="2" />
            <input type='submit' name='edit_dc_fan_input' value="Set" />
         </form>
         </td>
         </tr>
         <tr>
         <td>DC fan output adj.</td>
         <td> <?php echo $dc_fan_output . " %"; ?> </td>
         <td> <?php echo $dc_fan_output_ts; ?> </td>
                  <td>
         <form method="post">
            <input type="text" name="edit_dc_fan_output_set" size="2" />
            <input type='submit' name='edit_dc_fan_output' value="Set" />
         </form>
         </td>
         </tr>

         <tr>
         <td>HRC bypass</td>
         <td> <?php echo $hrc_bypass_temp . " *C"; ?> </td>
         <td> <?php echo $hrc_bypass_temp_ts; ?> </td>
         <td>
         <form method="post">
            <input type="text" name="edit_hrc_bypass_temp_set" size="2" />
            <input type='submit' name='edit_hrc_bypass_temp' value="Set" />
         </form>
         </td>
         </tr>
         <tr>
         <td>Input fan stop</td>
         <td> <?php echo $input_fan_stop_temp . " *C"; ?> </td>
         <td> <?php echo $input_fan_stop_temp_ts; ?> </td>
         <td>
         <form method="post">
            <input type="text" name="edit_input_fan_stop_temp_set" size="2" />
            <input type='submit' name='edit_input_fan_stop_temp' value="Set" />
         </form>
         </td>
         </tr>
         <tr>
         <td>Cell defrosting hysteresis</td>
         <td> <?php echo $cell_defrosting_hysteresis . " *C"; ?> </td>
         <td> <?php echo $cell_defrosting_hysteresis_ts; ?> </td>
         <td>
         <form method="post">
            <input type="text" name="edit_cell_defrosting_hysteresis_set" size="2" />
            <input type='submit' name='edit_cell_defrosting_hysteresis' value="Set" />
         </form>
         </td>
         </tr>
         <tr>
         <td>Incoming target temp</td>
         <td> <?php echo $incoming_target_temp . " *C"; ?> </td>
         <td> <?php echo $incoming_target_temp_ts; ?> </td>
         <td>
         <form method="post">
            <input type="text" name="edit_incoming_target_temp_set" size="2" />
            <input type='submit' name='edit_incoming_target_temp' value="Set" />
         </form>
         </td>
         </tr>
         <tr>
         <td>Pre heating temp</td>
         <td> <?php echo $pre_heating_temp . " *C"; ?> </td>
         <td> <?php echo $pre_heating_temp_ts; ?> </td>
         <td>
         <form method="post">
            <input type="text" name="edit_pre_heating_temp_set" size="2" />
            <input type='submit' name='edit_pre_heating_temp' value="Set" />
         </form>
         </td>
         </tr>

    </table>


    <table border="1">
    <caption>LTO efficiency</caption>
         <tr>
         <td>Incoming air</td>
          <td><?php echo  $incoming_air_efficiency . " %";  ?></td>
         </tr>
         <tr>
         <td>Outcoming air</td>
          <td><?php echo  $outcoming_air_efficiency . " %";  ?></td>
         </tr>
         <tr>
         <td>Average</td>
          <td><?php echo  round(($outcoming_air_efficiency + $incoming_air_efficiency) / 2, 1) . " %";  ?></td>
         </tr>

    </table>

    <table border="1">
    <caption>Dew point</caption>
         <tr>
          <td><?php echo $dew_point . " C*";?></td>
         </tr>
    </table>

    <table border="1">
    <caption>Radiator power</caption>
         <tr>
          <td><?php echo $radiator_watt . " W";?></td>
         </tr>
    </table>


    <table border="1">
    <caption>DS18B20 sensors</caption>
         <tr>
         <th>Sensor</th>
         <th>Value</th>
         <th>Updated</th>
         </tr>
         <tr>
         <td>Incoming air after radiator</td>
         <td> <?php echo $ds18b20_sensor1 . " *C"; ?> </td>
         <td> <?php echo $ds18b20_sensor1_ts; ?> </td>
         </tr>
         <tr> 
         <td>Exhaust air</td>
         <td> <?php echo $ds18b20_sensor2 . " *C"; ?> </td>
         <td> <?php echo $ds18b20_sensor2_ts; ?> </td>
         </tr>
    </table>

    <table border="1">
    <caption>AM2302 sensor</caption>
         <tr>
         <th>Sensor</th>
         <th>Value</th>
         <th>Updated</th>
         </tr>
         <tr>
         <td>Inside air temp</td>
         <td> <?php echo $am2302_temp . " *C"; ?> </td>
         <td> <?php echo $am2302_temp_ts; ?> </td>
         </tr>
         <tr> 
         <td>Inside air relative humidity</td>
         <td> <?php echo $am2302_rh . " %"; ?> </td>
         <td> <?php echo $am2302_rh_ts; ?> </td>
         </tr>
    </table>

</body>
</html>