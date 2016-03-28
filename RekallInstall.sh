#!/bin/bash
cd
svn checkout http://pdbparse.googlecode.com/svn/trunk/ pdbparse-read-only
cd pdbparse-read-only/
sudo python setup.py install
sudo apt-get install python-pefile mscompress cabextract python-pip virtualenv
sudo pip install construct
cd
