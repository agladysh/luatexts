<?php
// LUATEXTS PHP module (v. 0.1.3)
// https://github.com/agladysh/luatexts/
// Copyright (c) LUATEXTS authors. Licensed under the terms of the MIT license:
// https://github.com/agladysh/luatexts/tree/master/COPYRIGHT

class Luatexts
{
  private static function save_boolean($v)
  {
    return ($v) ? "1\n" : "0\n";
  }

  private static function save_integer($v)
  {
    return "N\n" . strval($v) . "\n";
  }

  private static function save_double($v)
  {
    return "N\n" . strval($v) . "\n";
  }

  private static function save_string($v)
  {
    return "8\n" . mb_strlen($v, 'UTF-8') . "\n" . $v . "\n";
  }

  private static function save_array($v)
  {
    $result = '';
    foreach ($v as $key => $value)
    {
      $result .= self::save_value($key) . self::save_value($value);
    }
    return 'T\n0\n' . count($v) . '\n' . $result;
  }

  private static function save_null($v)
  {
    return "-\n";
  }

  private static $supported_types = array(
      'boolean', 'integer', 'double', 'string', 'array', 'null'
    );

  private static function save_value($v)
  {
    $type = strtolower(gettype($v));
    $handler = 'save_' . $type;

    if (!in_array($type, self::$supported_types))
    {
      throw new Exception("luatexts does not support values of type \"$type\"");
    }

    return self::$handler($v);
  }

  public static function save()
  {
    $args = func_get_args();

    $result = count($args) . '\n';

    for($i = 0; $i < count($args); $i++)
    {
      $result .= self::save_value($args[$i]);
    }

    return $result;
  }
}
?>
