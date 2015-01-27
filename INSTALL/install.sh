#!/bin/sh

nodesfile="nodes"

sh create_nodes_cfg $nodesfile

sh moxa_master $nodesfile
sh moxa_master_cfg $nodesfile

sh moxa_slave $nodesfile
sh moxa_slave_cfg $nodesfile

sh moxa_wd $nodesfile
sh moxa_wd_lifes $nodesfile
sh moxa_rc_local $nodesfile
