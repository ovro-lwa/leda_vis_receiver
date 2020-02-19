#!/bin/bash

cd /home/sb/leda_vis_receiver-sb
export PBS_NODEFILE=/home/sb/leda_vis_receiver-sb/hosts.astm13
ipbs_taskfarm.py jobs2.astm13
