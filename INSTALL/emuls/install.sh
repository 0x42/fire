#!/bin/sh

# Файл nodes должен существовать в текущей директории
nodesfile="nodes"

sh moxa_uso $nodesfile
sh moxa_uso_cfg $nodesfile

sh moxa_pr $nodesfile
sh moxa_pr_cfg $nodesfile
