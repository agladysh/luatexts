<?php
// LUATEXTS PHP module (v. 0.1.5)
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

  // This function auto-converts all integer keys be one-based
  // (i.e. each integer key is incremented by one).
  private static function save_array($v)
  {
    $arr_size = 0;
    $hash_size = 0;

    $result = '';

    $max_arr_key = -1;
    for ($i = 0; ; ++$i)
    {
      if (isset($v[$i]))
      {
        $max_arr_key = $i;
        ++$arr_size;
        $result .= self::save_value($v[$i]);
      }
      else
      {
        break;
      }
    }

    foreach ($v as $key => $value)
    {
      if (strtolower(gettype($key)) == 'integer')
      {
        if ($key >= 0 && $key <= $max_arr_key)
        {
          continue; // Saved this value above
        }
        else
        {
          // Incrementing the key for data to be consistent.
          ++$key;
        }
      }

      $result .= self::save_value($key) . self::save_value($value);
      ++$hash_size;
    }

    return "T\n" . $arr_size . "\n" . $hash_size . "\n" . $result;
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
    $num_args = count($args);

    $result = $num_args . "\n";

    for ($i = 0; $i < $num_args; $i++)
    {
      $result .= self::save_value($args[$i]);
    }

    return $result;
  }
}
?>
