#!/bin/sh

nodesfile="nodes"

sh create_nodes_cfg $nodesfile

sh moxa_slave_cfg $nodesfile
