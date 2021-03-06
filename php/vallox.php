$g_id<html>
<head>
<title> Vallox control </title>
</head>
<body>
<?php 

$page = $_SERVER['PHP_SELF'];
$sec = "30";
$g_id = 0;

header("Refresh: $sec; url=$page");

function pp($arr)
{
    $retStr = '<ul>';
    if (is_array($arr)){
        foreach ($arr as $key=>$val){
            if (is_array($val)){
                $retStr .= '<li>' . $key . ' => ' . pp($val) . '</li>';
            }else{
                $retStr .= '<li>' . $key . ' => ' . $val . '</li>';
            }
        }
    }
    $retStr .= '</ul>';
    return $retStr;
}

function get_digit_vars($fp)
{
    $id = '{"id": $g_id++, "get":digit_vars}';
    fwrite($fp, $id);
    $str = fread($fp, 3000);
    $ob = json_decode($str, true);
    return $ob;
}

function get_digit_var_test($fp, $digit_vars,  $var_id, &$timestamp)
{
    $value = $digit_vars[$var_id]['value'];
    $timestamp = intval($digit_vars[$var_id]['ts']);
    $timestamp = date('r', $timestamp);
    return $value;
}

function get_ds18b20_vars($fp)
{
    $id = '{"id": $g_id++, "get" : ds18b20_vars}';
    fwrite($fp, $id);
    $str = fread($fp, 1500);
    $ob = json_decode($str, true);
    return $ob;
}

function get_control_vars($fp)
{
    $id = '{"id": $g_id++, "get":control_vars}';
    fwrite($fp, $id);
    $str = fread($fp, 1500);
    $ob = json_decode($str, true);
    return $ob;
}

function get_ds18b20_var($fp, $ds18b20_vars, $var_id, &$timestamp)
{
    $value = $ds18b20_vars[$var_id]['value'];
    $timestamp = intval($ds18b20_vars[$var_id]['ts']);
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

function get_ctrl_var($fp, $control_vars, $var_id)
{
    $value = $control_vars[$var_id]['value'];
    return $value;
}

function get_ctrl_var_ts($fp, $control_vars, $var_id, &$timestamp)
{
    $value = $control_vars[$var_id]['value'];
    $timestamp = intval($control_vars[$var_id]['ts']);
    $timestamp = date('r', $timestamp);    
    return $value;
}

function set_digit_var($fp, $var_id, $value)
{
    $str = '{"id": $g_id++, "set": {"digit_var":{ "'. $var_id . '": ' . $value . '}}}';
    fwrite($fp, $str);
}

function set_ctrl_var($fp, $var_id, $value)
{
    $str = '{"id": $g_id++, "set": {"control_var":{ "'. $var_id . '": ' . $value . '}}}';
    fwrite($fp, $str);
}


    $fp = fsockopen("udp://127.0.0.1", 8056, $errno, $errstr);

    if (!$fp) {
        echo "ERROR: $errno - $errstr<br />\n";
    }
    else if (isset($_POST['edit_cur_fan_speed'])) 
    {        
        set_digit_var($fp, "cur_fan_speed", $_POST['edit_cur_fan_speed_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();

    }
    else if (isset($_POST['edit_min_fan_speed'])) 
    {        
        set_digit_var($fp, "min_fan_speed", $_POST['edit_min_fan_speed_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_dc_fan_input'])) 
    {        
        echo $_POST['edit_dc_fan_input_set'];
        set_digit_var($fp, "dc_fan_input", $_POST['edit_dc_fan_input_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_dc_fan_output'])) 
    {        
        set_digit_var($fp, "dc_fan_output", $_POST['edit_dc_fan_output_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_hrc_bypass_temp'])) 
    {    
        set_digit_var($fp, "hrc_bypass_temp", $_POST['edit_hrc_bypass_temp_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_input_fan_stop_temp'])) 
    {        
        set_digit_var($fp, "input_fan_stop_temp", $_POST['edit_input_fan_stop_temp_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_cell_defrosting_hysteresis'])) 
    {        
        set_digit_var($fp, "cell_defrosting_hysteresis", $_POST['edit_cell_defrosting_hysteresis_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_incoming_target_temp'])) 
    {        
        set_digit_var($fp, "incoming_target_temp", $_POST['edit_incoming_target_temp_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_pressure_offset'])) 
    {        
        set_ctrl_var($fp, "pressure_offset", $_POST['edit_pressure_offset_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }           
    else if (isset($_POST['edit_defrost_max_duration'])) 
    {        
        set_ctrl_var($fp, "defrost_max_duration", $_POST['edit_defrost_max_duration_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_defrost_mode']))
    {
        set_ctrl_var($fp, "defrost_mode", $_POST['edit_defrost_mode_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_defrost_start_level'])) 
    {    
        set_ctrl_var($fp, "defrost_start_level", $_POST['edit_defrost_start_level_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_defrost_target_in_eff'])) 
    {    
        set_ctrl_var($fp, "defrost_target_in_eff", $_POST['edit_defrost_target_in_eff_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }
    else if (isset($_POST['edit_defrost_target_temp'])) 
    {    
        set_ctrl_var($fp, "defrost_target_temp", $_POST['edit_defrost_target_temp_set']);
        header("Location: " . $_SERVER['REQUEST_URI']);
        exit();
    }

    if (!$fp) 
    {
        echo "ERROR: $errno - $errstr<br />\n";
    }
    else
    {  
    $digit_vars = get_digit_vars($fp);
    $digit_vars = $digit_vars['digit_vars'];
    
    $ds18b20_vars = get_ds18b20_vars($fp);
    $ds18b20_vars = $ds18b20_vars['ds18b20_vars'];
       
    $control_vars = get_control_vars($fp);
    $control_vars = $control_vars['control_vars'];  
       
    $cur_fan_speed = get_digit_var_test($fp, $digit_vars, "cur_fan_speed", $cur_fan_speed_ts);
    
    

    $outside_temp = get_digit_var_test($fp, $digit_vars, "outside_temp", $outside_temp_ts);
    $exhaust_temp = get_digit_var_test($fp, $digit_vars, "exhaust_temp", $exhaust_temp_ts);
    
    $inside_temp = get_digit_var_test($fp, $digit_vars, "inside_temp", $inside_temp_ts);
    $incoming_temp = get_digit_var_test($fp, $digit_vars, "incoming_temp", $incoming_temp_ts);

    $pre_heating_temp = get_digit_var_test($fp, $digit_vars, "pre_heating_temp", $pre_heating_temp_ts);

    $post_heating_on_cnt = get_digit_var_test($fp, $digit_vars, "post_heating_on_cnt", $post_heating_on_cnt_ts);
    $post_heating_off_cnt = get_digit_var_test($fp, $digit_vars, "post_heating_off_cnt", $post_heating_off_cnt_ts);
    $incoming_target_temp = get_digit_var_test($fp, $digit_vars, "incoming_target_temp", $incoming_target_temp_ts);

    $panel_leds = get_digit_var_test($fp, $digit_vars, "panel_leds",  $panel_leds_ts);
    


    $min_fan_speed = get_digit_var_test($fp, $digit_vars, "min_fan_speed", $min_fan_speed_ts);
    $max_fan_speed = get_digit_var_test($fp, $digit_vars, "max_fan_speed", $max_fan_speed_ts);
    $hrc_bypass_temp = get_digit_var_test($fp, $digit_vars, "hrc_bypass_temp", $hrc_bypass_temp_ts);
    $input_fan_stop_temp = get_digit_var_test($fp, $digit_vars, "input_fan_stop_temp", $input_fan_stop_temp_ts);
    $cell_defrosting_hysteresis = get_digit_var_test($fp, $digit_vars, "cell_defrosting_hysteresis", $cell_defrosting_hysteresis_ts);
    $dc_fan_input = get_digit_var_test($fp, $digit_vars, "dc_fan_input", $dc_fan_input_ts);
    $dc_fan_output = get_digit_var_test($fp, $digit_vars, "dc_fan_output", $dc_fan_output_ts);
    $flags_2 = get_digit_var_test($fp, $digit_vars, "flag_2", $flags_2_ts);
    $flags_4 = get_digit_var_test($fp, $digit_vars, "flag_4", $flags_4_ts);
    $flags_5 = get_digit_var_test($fp, $digit_vars, "flag_5", $flags_5_ts);
    $flags_6 = get_digit_var_test($fp, $digit_vars, "flag_6", $flags_6_ts);
    
    $panel_leds = get_digit_var_test($fp, $digit_vars, "panel_leds", $panel_leds_ts);
    $IO_gate_3 = get_digit_var_test($fp, $digit_vars, "IO_gate_3", $IO_gate_3_ts);
    
    $rh1_sensor = get_digit_var_test($fp, $digit_vars, "rh1_sensor", $rh1_sensor_ts);
    $basic_rh_level = get_digit_var_test($fp, $digit_vars, "basic_rh_level", $basic_rh_level_ts);

    $ds18b20_sensor1 = get_ds18b20_var($fp, $ds18b20_vars, "ds_outside_temp", $ds18b20_sensor1_ts);
    $ds18b20_sensor2 = get_ds18b20_var($fp, $ds18b20_vars, "ds_exhaust_temp", $ds18b20_sensor2_ts);
    $ds18b20_sensor3 = get_ds18b20_var($fp, $ds18b20_vars, "ds_incoming_temp", $ds18b20_sensor3_ts);
  
    $am2302_temp = get_am2302_var($fp, AM2302_TEMP, $am2302_temp_ts);
    $am2302_rh = get_am2302_var($fp, AM2302_RH, $am2302_rh_ts);

    $post_heating_on_time_total = get_ctrl_var($fp, $control_vars, "post_heating_time");
    $pre_heating_on_time_total = get_ctrl_var($fp,$control_vars, "pre_heating_time");
    $defrost_on_time_total = get_ctrl_var($fp, $control_vars, "defrost_time");
    $defrost_on_time = get_ctrl_var($fp, $control_vars, "defrost_on_time");
    
    
    $pressure_offset = get_ctrl_var($fp, $control_vars, "pressure_offset");
    $defrost_mode = get_ctrl_var($fp, $control_vars, "defrost_mode");
    

    $defrost_max_duration = get_ctrl_var($fp, $control_vars, "defrost_max_duration");
    $defrost_start_level = get_ctrl_var($fp, $control_vars, "defrost_start_level");
    $defrost_target_in_eff = get_ctrl_var($fp, $control_vars, "defrost_target_in_eff");
    $defrost_target_temp = get_ctrl_var($fp, $control_vars, "defrost_target_temp");
    
    
    $t = floatval($inside_temp);
    $rh = $rh1_sensor / 100; 
    $a = floatval(17.27);
    $b = floatval(237.7);
    $z =  ((($a * $t) / ($b + $t)) + log($rh));
    $dew_point = get_ctrl_var($fp, $control_vars, "dew_point");
    $pressure_outdoor = get_ctrl_var_ts($fp, $control_vars, "pressure_outdoor", $pressure_outdoor_ts);
    $pressure_indoor = get_ctrl_var_ts($fp, $control_vars, "pressure_indoor", $pressure_indoor_ts);
    $pressure_diff = get_ctrl_var_ts($fp, $control_vars, "pressure_diff", $pressure_diff_ts);    
    $pressure_diff_filtered = get_ctrl_var_ts($fp, $control_vars, "pressure_diff_filtered", $pressure_diff_filtered_ts);  
    
    
    $air_flow = 15 + 10 * $cur_fan_speed;
    $radiator_watt = round($air_flow * 1.225 * ($ds18b20_sensor1 - $outside_temp),1); 
    $incoming_air_efficiency = get_ctrl_var($fp, $control_vars, "in_efficiency");
    $outcoming_air_efficiency = get_ctrl_var($fp, $control_vars, "out_efficiency");
    $incoming_air_efficiency_filtered = get_ctrl_var($fp, $control_vars, "in_efficiency_filtered");
    $outcoming_air_efficiency_filtered = get_ctrl_var($fp, $control_vars, "out_efficiency_filtered");    
    
    $incoming_air_efficiency_2 = round(floatval((floatval($ds18b20_sensor3) - floatval($outside_temp)) / (floatval($inside_temp) - floatval($outside_temp))) * 100,1);
    $outcoming_air_efficiency_2 = round(floatval((floatval($inside_temp) - floatval($exhaust_temp)) / (floatval($inside_temp) - floatval($outside_temp))) * 100,1);
    fclose($fp);
}
?> 



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
         <td>Leds</td>
         <td> <?php echo pp($panel_leds); ?> </td>
         <td> <?php echo $panel_leds_ts; ?> </td>
         </tr>
         <td>I/O gate 3</td>
         <td> <?php echo pp($IO_gate_3); ?> </td>
         <td> <?php echo $IO_gate_3_ts; ?> </td>
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
         <td>MAX FAN speed</td>
         <td> <?php echo $max_fan_speed; ?> </td>
         <td> <?php echo $max_fan_speed_ts; ?> </td>
         <td>
         <form method="post">
            <input type="number" name="edit_max_fan_speed_set" size="2" />
            <input type='submit' name='edit_max_fan_speed' value="Set" />
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
    <caption>LTO efficiency (after radiator)</caption>
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
    <caption>LTO efficiency filtered (after radiator)</caption>
         <tr>
         <td>Incoming air</td>
          <td><?php echo  $incoming_air_efficiency_filtered . " %";  ?></td>
         </tr>
         <tr>
         <td>Outcoming air</td>
          <td><?php echo  $outcoming_air_efficiency_filtered . " %";  ?></td>
         </tr>
         <tr>
         <td>Average</td>
          <td><?php echo  round(($outcoming_air_efficiency_filtered + $incoming_air_efficiency_filtered) / 2, 1) . " %";  ?></td>
         </tr>
    </table>    
    
    <table border="1">
    <caption>LTO efficiency (outside temp) </caption>
         <tr>
         <td>Incoming air</td>
          <td><?php echo  $incoming_air_efficiency_2 . " %";  ?></td>
         </tr>
         <tr>
         <td>Outcoming air</td>
          <td><?php echo  $outcoming_air_efficiency_2 . " %";  ?></td>
         </tr>
         <tr>
         <td>Average</td>
          <td><?php echo  round(($outcoming_air_efficiency_2 + $incoming_air_efficiency_2) / 2, 1) . " %";  ?></td>
         </tr>

    </table>

    <table border="1">
    <caption>Dew point</caption>
         <tr>
          <td><?php echo $dew_point . " C*";?></td>
         </tr>
    </table>

    <table border="1">
    <caption>Pressures</caption>
         <tr>
         <td>Outdoor</td>
          <td><?php echo  $pressure_outdoor . " Pa";  ?></td>
          <td> <?php echo $pressure_outdoor_ts; ?> </td>
         </tr>
         <tr>
         <td>Indoor</td>
          <td><?php echo  $pressure_indoor . " Pa";  ?></td>
          <td> <?php echo $pressure_indoor_ts; ?> </td>          
         </tr>
         <tr>
         <td>Difference</td>
          <td><?php echo  $pressure_diff . " Pa";  ?></td>
          <td> <?php echo $pressure_diff_ts; ?> </td>
        </tr>
         <tr>
         <td>Difference filtered</td>
          <td><?php echo  $pressure_diff_filtered . " Pa";  ?></td>
          <td> <?php echo $pressure_diff_filtered_ts; ?> </td>
        </tr>        
        <tr>
        <td>Pressure offset</td>
        <td> <?php echo $pressure_offset . " Pa"; ?> </td>
        <td>
            <form method="post">
                <input type="text" name="edit_pressure_offset_set" size="2" />
                <input type='submit' name='edit_pressure_offset' value="Set" />
            </form>
        </td>
        </tr>
         
    </table>    
    
    <table border="1">
    <caption>Radiator power</caption>
         <tr>
          <td><?php echo $radiator_watt . " W";?></td>
         </tr>
    </table>


    <table border="1">
    <caption>Post heating 1000 w</caption>
         <tr>
          <th>Time total</th>
          <th>kWh total</th>
          <th>eur total</th>
          </tr>
          <tr>
          <td><?php echo $post_heating_on_time_total . " sec";?></td>
          <td><?php echo round(($post_heating_on_time_total / 3600), 2) . " kWh";?></td>
          <td><?php echo round((($post_heating_on_time_total / 3600) * 0.12), 2) . " eur";?></td>
         </tr>
    </table>

   <table border="1">
    <caption>Pre heating 1000 w</caption>
         <tr>
          <th>Time total</th>
          <th>kWh total</th>
          <th>eur total</th>
          </tr>
          <tr>
          <td><?php echo $pre_heating_on_time_total . " sec";?></td>
          <td><?php echo round((($pre_heating_on_time_total / 3600)), 2) . " kWh";?></td>
          <td><?php echo round(((($pre_heating_on_time_total / 3600))  * 0.12), 2) . " eur";?></td>
         </tr>
    </table>


   <table border="1">
    <caption>Defrost resistor 1000 w</caption>
         <tr>
          <th>Time total</th>
          <th>kWh total</th>
          <th>eur total</th>
          </tr>
          <tr>
          <td><?php echo $defrost_on_time_total . " sec";?></td>
          <td><?php echo round((($defrost_on_time_total / 3600) * 1.5), 2) . " kWh";?></td>
          <td><?php echo round(((($defrost_on_time_total / 3600) * 1.5)  * 0.12), 2) . " eur";?></td>
         </tr>
    </table>
    
   <table border="1">
    <caption>Defrost control</caption>
        <tr>
        <td>Defrost mode</td>
        <td> <?php echo $defrost_mode; ?> </td>
        <td>
            <form method="post">
                <input type="radio" name="edit_defrost_mode_set" value="0" /> Off
                <input type="radio" name="edit_defrost_mode_set" value="1" /> On
                <input type="radio" name="edit_defrost_mode_set" value="2" /> Auto
                <input type='submit' name='edit_defrost_mode' value="Set" />
            </form>
        </td>
        </tr>           
        <tr>
        <td>Defrost time</td>
        <td> <?php echo $defrost_on_time . " s"; ?> </td>
        </tr> 
        <tr>
        <td>Start level (LTO efficiency incoming air)</td>
        <td> <?php echo $defrost_start_level . " %"; ?> </td>
        <td>
            <form method="post">
                <input type="text" name="edit_defrost_start_level_set" size="2" />
                <input type='submit' name='edit_defrost_start_level' value="Set" />
            </form>
        </td>
        </tr>
        <tr>
        <td>Max duration of defrost</td>
        <td> <?php echo $defrost_max_duration . " min"; ?> </td>
        <td>
            <form method="post">
                <input type="text" name="edit_defrost_max_duration_set" size="2" />
                <input type='submit' name='edit_defrost_max_duration' value="Set" />
            </form>
        </td>
        </tr>        
        <tr>
        <td>Target incoming efficiency</td>
        <td> <?php echo $defrost_target_in_eff . " %"; ?> </td>
        <td>
            <form method="post">
                <input type="text" name="edit_defrost_target_in_eff_set" size="2" />
                <input type='submit' name='edit_defrost_target_in_eff' value="Set" />
            </form>
        </td>
        </tr>
        <td>Target incoming temperature</td>
        <td> <?php echo $defrost_target_temp . " *C"; ?> </td>
        <td>
            <form method="post">
                <input type="text" name="edit_defrost_target_temp_set" size="2" />
                <input type='submit' name='edit_defrost_target_temp' value="Set" />
            </form>
        </td>
        </tr>
    </table -->    
    
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
         </tr>
         <tr> 
         <td>Incoming air</td>
         <td> <?php echo $ds18b20_sensor3 . " *C"; ?> </td>
         <td> <?php echo $ds18b20_sensor3_ts; ?> </td>
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
