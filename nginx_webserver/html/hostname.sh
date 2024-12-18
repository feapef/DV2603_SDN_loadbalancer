#!/bin/bash
echo "<html>"
echo "<head><title>Host Information</title></head>"
echo "<body>"
echo "<h1>Computer Name: $(hostname)</h1>"
echo "<h1>uname : $(uname -a)</h1>"
echo "</body>"
echo "</html>"
