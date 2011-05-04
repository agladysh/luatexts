<?php

class Luatexts{
  private static function save_boolean($v){
    return ($v) ? "1\n" : "0\n";
  }
  private static function save_integer($v){
    return "N\n" . strval($v) . "\n";
  }
  private static function save_double($v){
    return "N\n" . strval($v) . "\n";
  }
  private static function save_string($v){
    return "U\n" . mb_strlen($v, 'UTF-8') . "\n" . $v . "\n";
  }
  private static function save_array($v){
    $result = "";
    foreach ($v as $key => $value){
      $result .= self::save($key) . self::save($value);
    }
    return "T\n0\n" . count($v) . "\n" . $result;
  }
  private static function save_null($v){
    return "-\n";
  }

  private static $supported_types = array(
    'boolean', 'integer', 'double', 'string', 'array', 'null'
  );

  public static function save(){
    $args = func_get_args();

    if (empty($args)) return '';
    $result = '';
    foreach($args as $v){
      $type = strtolower(gettype($v));
      $handler = 'save_'.$type;

      if (!in_array($type, self::$supported_types) || !is_callable('Luatexts', $handler)){
        throw new Exception("luatexts does not support values of type " . $type);
      }
      $result .= self::$handler($v);
    }
    return $result;
  }
}
?>