KConfig is a system for loading text configuration files. It's used
for parsing all the .txt config files in the knights_data directory.

It uses the GOLD Parsing System [1]. However I wrote the "engine"
myself (see gold_parser.hpp/.cpp) since the engines provided with GOLD
were quite poor, I found (at least the ones I tried were).
