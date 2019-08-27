# ModelExplorer v2.1

ModelExplorer is a metabolic model visualization package that can assist the user in finding blocked parts of the metabolic network as well as finding out why they are blocked. For this, the user is provided with three different definitions of inactive reactions: *FBA* and *Bidirectional* for correcting finished models, and *Dynamic* for building new ones. ModelExplorer assists the user in finding the source of network blocking with two tracking tools – one which shows the node’s nearest neighbours, and one that shows a minimal reaction path necessary to produce a metabolite. The user can also find and separately visualize connected sets of blocked reactions and metabolites – Blocked Modules. Blocked reactions and non-produced metabolites can often be the result of faulty transport between compartments. The network layout algorithm used in ModelExplorer therefore visually separates and highlights different cellular compartments, in order to make troubleshooting easier and more intuitive. Using the inbuilt ErrorTracer algorithm (also provided as a command line tool) the user can explicitly trace the origins of all reaction inconsistencies. Finally ModelExplorer has all the necessary tools required to edit existing models and build up new ones by hand.

## Installation

The program is provided as two different executables, one being the graphical software ModelExplorer, and the other being a command line tool running the ErrorTracer algorithm. The programs come in two versions: Windows and Linux. The Windows versions have been tested to run on Windows 10, while the Linux verision has been tested to run on Ubuntu 17.04, 17.10, 16.04 LTS and 18.04 LTS, as well as on Manjaro 17.1.1. We recommend using a true installation of the system and not a virtual machine, as the latter will likely not be able to take advantage of the GPU. 

On Windows no installation is required. The only premise is that the ModelExplorer (or ErrorTracer) executable is kept in the same folder as the libraries and the font file. The user is advised to place the content of the archive into a single folder and make a shortcut to the ModelExplorer executable. Alternatively the program can be run from a terminal (cmd) window, which is advisable in case troubleshooting will be required. 

The Linux installation procedure works as follows:
Extract the archive to a folder of your choice. Run the installation script, agreeing to everything that the user will be prompted with (it will preform a system update and install the required libraries, copying the libraries shipped with ModelExplorer into a system folder). 

```
$ sudo ./install.sh
```

After the installation script has been run, ModelExplorer can be launched from within a terminal or directly. Running it from terminal is advisable in case troubleshooting will be required. 

## Compiling

The program is shipped together with its source code located in the “source_code” folder, which can be compiled on Linux using Cmake. In order to compile the program, ***the user first needs to perform the installation steps in the previous section***. Next, the user needs to enter the build folder inside the source folder: 

```
$ cd <your-ModelExplorer-folder>/source_code/build
```

The code can then be compiled using the following two commands:

```
$ cmake ..
$ make 
```

The executable “ModelExplorer” will then be located in the “build” folder and will require the “arial.ttf” font file to be located in the same folder with the executable. 

The same compilation procedure as above applies to the ErorTracer executable, except its source code is located in the “source_code_ErrorTracer” folder. The font file is not required to run ErrorTracer standalone executable. 

## Launching

The program should be run from the terminal and the command could be followed by a path to a model file that is to be opened. For example on Linux:

```
$ ./ModelExplorer test.xml
```

Or on Windows:

```
$ ModelExplorer.exe test.xml
```

The program accepts only **SBML2** as the model format. A model could be loaded either with the command above or from within the GUI. If reactions refer to undefined metabolites, undefined compartments or lack stoichiometries, the user will be presented with self-explanatory warnings. If species, reactions or compartments lack the “id” field, an appropriate error message will be presented, and the model will not be opened. When the model gets loaded the user is presented (in the terminal) with model name, flux unit and the number of chemical species and reactions in the model:

When a model is loaded, the program will immediately proceed to calculating which metabolites and reactions are blocked in the three available blocking modes. This and the following graphical layout procedure may take some time, especially for models with more than 3000 reactions / metabolites. 

For certain models with a lot of metabolites that are not utilized in any reaction (e.g. Recon2), it is recommended to use the ***-ps*** flag before the path to the model in order to remove these species and decrease the time it takes to make a layout of the model. In case of very large models (with connected parts >104 reactions) it may take a long time to run all the three algorithms, so the user can use 
***-FBAonly*** flag to save time and only do the FBA (and error) calculations, omitting the Bidirectional and Dynamic ones. 

If the user wants to test the execution time of the different algorithms on his or her computer, the flag
***-time*** should be added. In order to test the graphical performance of ModelExplorer the flag ***-FPS*** will output the frames per second of the visualization. Note that the latter command will only show output if the user is actively navigating, as otherwise the screen is not refreshed.

All of the above commands also apply to the ErrorTracer executable, with the addition of the ***-o*** flag, which should be followed by the folder of file name to which the error tracking output of the ErrorTracer should be saved. If this flag is not specified, the output will be saved.

## Usage

Please consult the user manual for a full description of the capabilites of the software. Videos demonstrating the capabilities of ModelExplorer are available on [YouTube](https://www.youtube.com/watch?v=xWNrZN6jeoA&list=PLYa7LEU4_U3daU7E6AZUrsLCGrruSUGJm&index=1). 

## Licence

The software is provided as a binary executable with the corresponding source code under the Eclipse Public License v2.0, a copy of which can be found in LICENCE.md.
