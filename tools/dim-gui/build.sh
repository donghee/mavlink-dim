#!/bin/sh

make && git add . &&  git commit -a -m "update `date +%F-%T`"
