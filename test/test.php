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
  $required = "T\n0\n3\nU\n3\nobj\nT\n0\n0\nU\n5\narray\nT\n0\n8\nN\n0\nN\n0.5\nN\n1\nN\n1\nN\n2\nU\n4\nnull\nN\n3\n-\nN\n4\nU\n9\nundefined\nN\n5\n1\nN\n6\n0\nN\n7\nT\n0\n0\nU\n3\nutf\nU\n11\nЭЭХ! Naïve?\n";
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
  $required = "N\n123\nU\n11\ntest string\nT\n0\n2\nN\n0\nN\n1\nU\n4\ntest\nN\n123\n";
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