
echo "json1"
../serialNgram json1 > json1.output
diff <( tr -d ' \n' <json1 ) <( tr -d ' \n' <json1.output)

echo "json2"
../serialNgram json2 > json2.output
diff <( tr -d ' \n' <json1 ) <( tr -d ' \n' <json1.output)

echo "json3"
../serialNgram json3 > json3.output
diff <( tr -d ' \n' <json3 ) <( tr -d ' \n' <json3.output)

echo "json4"
../serialNgram json4 > json4.output
diff <( tr -d ' \n' <json4 ) <( tr -d ' \n' <json4.output)

echo "json5"
../serialNgram json5 > json5.output
diff <( tr -d ' \n' <json5 ) <( tr -d ' \n' <json5.output)

echo "json6"
../serialNgram json6 > json6.output
diff <( tr -d ' \n' <json6 ) <( tr -d ' \n' <json6.output)

echo "inputTemplate"
../serialNgram inputTemplate > inputTemplate.output
diff <( tr -d ' \n' <inputTemplate ) <( tr -d ' \n' <inputTemplate.output)
