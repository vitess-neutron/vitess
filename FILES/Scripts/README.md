# Scripts folder

Contains an example on how to use python to plot monitor 2D data. 

 - `rshow.py`: script to parse and plot 2D monitor data from vitess.
 - `test_rshow.py`: test example on data passed by argument.
 - `mon2D_data.dat`: dummy data to test the functions in `rshow.py`

## Usage example

```{bash}
$ python test_rshow.py mon2D_data.dat
```

will generate two outputs, one using the function defines in `rshow.py`, and the other one using the script written in `test_rshow.py`.