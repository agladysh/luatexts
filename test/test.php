<?php
chdir('../');
require_once('src/php/luatexts.php');

class Test{
}

try{
  $a = array(
        "obj" => array(),
        "array" => array(
            0.5, 1, 'null', null, 'undefined', true, false, array()
        ),
        "utf" => "ЭЭХ! Naïve?"
  );
  print_r($a);
  $required = array("1",
  "T",
  "0",
  "3",
  "8",
  "3",
  "obj",
  "T",
  "0",
  "0",
  "8",
  "5",
  "array",
  "T",
  "0",
  "8",
  "N",
  "0",
  "N",
  "0.5",
  "N",
  "1",
  "N",
  "1",
  "N",
  "2",
  "8",
  "4",
  "null",
  "N",
  "3",
  "-",
  "N",
  "4",
  "8",
  "9",
  "undefined",
  "N",
  "5",
  "1",
  "N",
  "6",
  "0",
  "N",
  "7",
  "T",
  "0",
  "0",
  "8",
  "3",
  "utf",
  "8",
  "11",
  "ЭЭХ! Naïve?");
  $required = implode("\n", $required)."\n";
  $lt_result = Luatexts::save($a);
  echo "RESULT: " . ($required == $lt_result ? "OK\n" : "ERROR\n");

  if ($required != $lt_result){
    $required = explode("\n", $required);
    $lt_result = explode("\n", $lt_result);

    $count = max(count($required), count($lt_result));
    for($i=0;$i<$count;$i++){
      echo @$required[$i]."\t".@$lt_result[$i].(@$required[$i] != @$lt_result[$i] ? "\t<< ERROR\n" : "\n");
    }
  }

  print("\n\n");
  print("Serialize many arguments: 123, 'test string', array(1, 'test' => 123)\n");
  $lt_result = Luatexts::save(123, 'test string', array(1, 'test' => 123));
  $required = array("3",
  "N",
  "123",
  "8",
  "11",
  "test string",
  "T",
  "0",
  "2",
  "N",
  "0",
  "N",
  "1",
  "8",
  "4",
  "test",
  "N",
  "123");
  $required = implode("\n", $required)."\n";
  echo "RESULT: " . ($required == $lt_result ? "OK\n" : "ERROR\n");

  if ($required != $lt_result){
    $required = explode("\n", $required);
    $lt_result = explode("\n", $lt_result);

    $count = max(count($required), count($lt_result));
    for($i=0;$i<$count;$i++){
      echo @$required[$i]."\t".@$lt_result[$i].(@$required[$i] != @$lt_result[$i] ? "\t<< ERROR\n" : "\n");
    }
  }
  

  print("\n\n");
  $a = array(
        "obj" => array(),
        "array" => array(
            0.5, 1, 'null', null, 'undefined', true, false, array()
        ),
        "utf" => "ЭЭХ! Naïve?",
        new Test
  );
  print_r($a);
  $lt_result = Luatexts::save($a);
} catch (Exception $e) {
  echo "\nEXTENSION: ".$e->getMessage()."\n";
}
?>
