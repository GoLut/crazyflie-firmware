#Customised for the Msc. Thesis of Jasper-Jan Lut

###Custom folders:
- **src\lib\Custom_Libs**: This folder contains all custom and external libraries.

- **src\deck\drivers\src\colordeck.c**:
This file is the backbone for the custom Crazyflie (hardware) Deck: "The ColourDeck". 

NOTE: The code requires a working deck mounted underneeth the crazyflie. In combination with a working and calibrated Lighthouse deck.


### Other repositories this work requires:
- MSC_Thesis_Jasper_Jan_Lut_2023_TuDelft: [link](https://github.com/GoLut/MSC_Thesis_Jasper_Jan_Lut_2023_TuDelft.git)
- The modified Crazyflie client Python code: [link](https://github.com/GoLut/crazyflie-clients-python)

# Crazyflie Firmware  [![CI](https://github.com/bitcraze/crazyflie-firmware/workflows/CI/badge.svg)](https://github.com/bitcraze/crazyflie-firmware/actions?query=workflow%3ACI)

This project contains the source code for the firmware used in the Crazyflie range of platforms, including the Crazyflie 2.X and the Roadrunner.

### Crazyflie 1.0 support

The 2017.06 release was the last release with Crazyflie 1.0 support. If you want
to play with the Crazyflie 1.0 and modify the code, please clone this repo and
branch off from the 2017.06 tag.

## Building and Flashing
See the [building and flashing instructions](https://github.com/bitcraze/crazyflie-firmware/blob/master/docs/building-and-flashing/build.md) in the github docs folder.


## Official Documentation

Check out the [Bitcraze crazyflie-firmware documentation](https://www.bitcraze.io/documentation/repository/crazyflie-firmware/master/) on our website.

## Generated documentation

The easiest way to generate the API documentation is to use the [toolbelt](https://github.com/bitcraze/toolbelt)

```tb build-docs```

and to view it in a web page

```tb docs```

## Contribute
Go to the [contribute page](https://www.bitcraze.io/contribute/) on our website to learn more.

### Test code for contribution

To run the tests please have a look at the [unit test documentation](https://www.bitcraze.io/documentation/repository/crazyflie-firmware/master/development/unit_testing/).

## License

The code is licensed under LGPL-3.0
