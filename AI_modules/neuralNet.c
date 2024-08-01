/**
 * MODULE: neuralNet
 * FILE: neuralNet.c
 * VERSION: 1.0.0
 * HISTORICAL: Created by Eloy Urriens on 30/7/2024
 * DESCRIPTION: This module can build a neural network.
 * CC: BY SA
 */

#include "dynamicListMatrix.h"
#include "neuralNet.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

/**
 * FUNCTION: errorNeuralNet
 * INPUT: error message
 * REQUIREMENTS: None
 * MODIFIES: Finish the program.
 */
void errorNeuralNet(char error[]) {
    printf("\n\n\nERROR in the module neuralNet: %s\n", error);
    while (true)
        exit(-1);
}

/**
 * FUNCTION: newNeuralNet
 * INPUT: 
 *      The array of the number of neurons per layer. It's length is the
 *      number of the layers. Example:
 *          If there are 3 layers and the first has 5 neurons, the second
 *          layer has 2 neurons and the last layer has 1 neuron, this array
 *          is: {5, 2, 1}. {input layer, hidden layer, output layer}.
 *      The array of the activate function per layer. It's length is the
 *      number of the layers - 1, because the input layer hasn't activate
 *      function. Example:
 *          If there are 3 layers and the first don't use activate function, the 
 *          second use the sigmoide and the last use the relu, this array
 *          is: {sigmoide, relu}
 *          First layer -> None
 *          Second layer -> Sigmoide
 *          Third layer -> Relu
 *          relu, sigmoide and tanh are constants.
 *      The array of characters, which is the description.
 * REQUIREMENTS:
 *      The length the last array <= MAX_DESCRIPTION
 *      The number of neurons per layer <= MAX_NEURONS
 *      The description must have '\0'.
 * OUTPUT:
 *      A neural network (NeuralNet).
 */
void newNeuralNet(NeuralNet *net, unsigned char layers[], unsigned char actv_funcs[], char desc[MAX_DESCRIPTION], unsigned char n_layers) {
    if (n_layers < 2) {
        errorNeuralNet("The neural network cannot be created because there are fewer than 2 layers.");
    }
    
    // We create the neural network
    newDynamicListLayer(&net->layers);

    int i;
    Layer layer;
    newLayer(&layer, 1, 1, 3);
    for (i = 1; i < n_layers; i++) {
        newLayer(&layer, layers[i - 1], layers[i], actv_funcs[i - 1]);
        appendDynamicListLayer(&net->layers, layer);
    }
    net->n_layers = n_layers;
    net->n_inputs = layers[0];
    net->n_outputs = layers[n_layers - 1];

    i = 0;
    while (i < MAX_DESCRIPTION - 1 && desc[i] != '\0') {
        i++;
    }

    if (desc[i] != '\0') {
        desc[i] = '\0';
    }
    strcpy(net->description, desc);
}

/**
 * FUNCTION: calculatePrediction
 * INPUT: input (Matrix) and a net (NeuralNet)
 * REQUIREMENTS: None.
 * OUTPUT: The outputs
 */
void calculatePrediction(dynamicListMatrix *outputs, Matrix input, NeuralNet net) {
    Matrix w, b, z, aux;
    Matrix previous_output, current_output;
    Layer layer;

    appendDynamicListMatrix(outputs, input);
    for (int i = 1; i < net.n_layers; i++) {
        consultElemDynamicListLayer(&layer, net.layers, i - 1);
        consultElemDynamicListMatrix(&previous_output, *outputs, i - 1);

        getWeights(&w, layer);
        multiplyMatrix(&aux, previous_output, w);

        getBias(&b, layer);
        addMatrix(&z, aux, b);
        
        activateFunction(&current_output, z, layer);
        appendDynamicListMatrix(outputs, current_output);
    }
}

unsigned int calculateAlphaEpoch(unsigned int n_epochs) {
    unsigned int alpha_epoch;

    alpha_epoch = (unsigned int) (n_epochs * 0.1);
    if (alpha_epoch == 0) {
        alpha_epoch = 1;
    }

    if (alpha_epoch > 250) {
        alpha_epoch = 250;
    }

    return alpha_epoch;
}

void trainWithStopOverfitting(NeuralNet *net, float *init_MSE, float *end_MSE, float *min_MSE, unsigned *n_epochs_completed, Matrix inT, Matrix inNT, Matrix outT, Matrix outNT, unsigned int n_epochs, float lr) {
    Matrix deriv_mse, deriv_act_func, dC_dw;
    dynamicListMatrix outputs;
    float MSE1_overffiting, MSE2_overffiting;
    float aux_MSE;

    newDynamicListMatrix(&outputs);
    if (getNumberLayers(*net) == 2) {
        Matrix delta, output;
        Layer layer;
        unsigned int alpha_epoch;
        int i;

        alpha_epoch = calculateAlphaEpoch(n_epochs);

        calculatePrediction(&outputs, inNT, *net);
        consultElemDynamicListMatrix(&output, outputs, 1);
        MSE1_overffiting = MSEMatrix(output, outNT);
        MSE2_overffiting = MSE1_overffiting;
        freeDynamicListMatrix(&outputs);

        calculatePrediction(&outputs, inT, *net);
        consultElemDynamicListMatrix(&output, outputs, 1);
        *init_MSE = MSEMatrix(output, outT);
        *min_MSE = *init_MSE;

        i = 0;
        while (i < n_epochs && MSE2_overffiting - MSE1_overffiting <= 0) {
            consultElemDynamicListLayer(&layer, net->layers, 0);

            // The errors are calculated
            derivMSEMatrix(&deriv_mse, output, outT);
            derivActivateFunction(&deriv_act_func, output, layer);
            multiplyNumbersMatrix(&delta, deriv_mse, deriv_act_func);
            
            // Gradient descendent
            optimizeBias(&layer, delta, lr);
            transposeMatrix(&delta);
            multiplyMatrix(&dC_dw, delta, inT);
            optimizeWeights(&layer, dC_dw, lr);

            changeElemDynamicListLayer(&net->layers, 0, layer);
            
            // Training prediction error with inNT (input_not_train)
            freeDynamicListMatrix(&outputs);
            calculatePrediction(&outputs, inNT, *net);
            consultElemDynamicListMatrix(&output, outputs, 1);
            MSE1_overffiting = MSE2_overffiting;
            MSE2_overffiting = MSEMatrix(output, outNT);

            // Training prediction error with inT (input_train)
            freeDynamicListMatrix(&outputs);
            calculatePrediction(&outputs, inT, *net);
            consultElemDynamicListMatrix(&output, outputs, 1);
            aux_MSE = MSEMatrix(output, outT);
            if (aux_MSE < *min_MSE) {
                *min_MSE = aux_MSE;
            }
            
            i++;
        }
        consultElemDynamicListMatrix(&output, outputs, 1);
        *end_MSE = MSEMatrix(output, outT);
        *n_epochs_completed = i;
    }
    else { // > 2
        Matrix delta_1, delta_2, aux_w, aux;
        Matrix output;
        Layer current_layer, previous_layer;
        int i, alpha_epoch;

        alpha_epoch = calculateAlphaEpoch(n_epochs);

        calculatePrediction(&outputs, inNT, *net);
        consultElemDynamicListMatrix(&output, outputs, net->n_layers - 1);
        MSE1_overffiting = MSEMatrix(output, outNT);
        MSE2_overffiting = MSE1_overffiting;
        freeDynamicListMatrix(&outputs);

        calculatePrediction(&outputs, inT, *net);
        consultElemDynamicListMatrix(&output, outputs, net->n_layers - 1);
        *init_MSE = MSEMatrix(output, outT);
        *min_MSE = *init_MSE;

        i = 0;
        while (i < n_epochs && MSE2_overffiting - MSE1_overffiting <= 0) {

            // Backpropagation and gradient descendent (Train)
            consultElemDynamicListLayer(&current_layer, net->layers, net->n_layers - 2);

            derivMSEMatrix(&deriv_mse, output, outT);
            derivActivateFunction(&deriv_act_func, output, current_layer);
            multiplyNumbersMatrix(&delta_1, deriv_mse, deriv_act_func);
            for (int j = net->n_layers - 2; j > 0; j--) {
                consultElemDynamicListMatrix(&output, outputs, j);
                consultElemDynamicListLayer(&current_layer, net->layers, j);
                consultElemDynamicListLayer(&previous_layer, net->layers, j - 1);

                getWeights(&aux_w, current_layer);
                transposeMatrix(&aux_w);

                derivActivateFunction(&deriv_act_func, output, previous_layer);
                multiplyMatrix(&aux, delta_1, aux_w);
                multiplyNumbersMatrix(&delta_2, aux, deriv_act_func);

                transposeMatrix(&output);
                multiplyMatrix(&dC_dw, output, delta_1);
                optimizeWeights(&current_layer, dC_dw, lr);
                optimizeBias(&current_layer, delta_1, lr);

                changeElemDynamicListLayer(&net->layers, j, current_layer);

                delta_1 = delta_2;
            }
            consultElemDynamicListMatrix(&output, outputs, 0);
            consultElemDynamicListLayer(&current_layer, net->layers, 0);

            transposeMatrix(&output);
            multiplyMatrix(&dC_dw, output, delta_1);
            optimizeWeights(&current_layer, dC_dw, lr);
            optimizeBias(&current_layer, delta_1, lr);

            changeElemDynamicListLayer(&net->layers, 0, current_layer);

            // Check overffiting point.
            freeDynamicListMatrix(&outputs);
            if ((i + 1) % alpha_epoch == 0) {
                calculatePrediction(&outputs, inNT, *net);
                consultElemDynamicListMatrix(&output, outputs, net->n_layers - 1);
                MSE1_overffiting = MSE2_overffiting;
                MSE2_overffiting = MSEMatrix(output, outNT);
            }

            // Calculate the error
            freeDynamicListMatrix(&outputs);
            calculatePrediction(&outputs, inT, *net);
            consultElemDynamicListMatrix(&output, outputs, net->n_layers - 1);
            aux_MSE = MSEMatrix(output, outT);
            if (aux_MSE < *min_MSE) {
                *min_MSE = aux_MSE;
            }

            i++;
        }
        consultElemDynamicListMatrix(&output, outputs, net->n_layers - 1);
        *end_MSE = MSEMatrix(output, outT);
        *n_epochs_completed = i;
    }
    freeDynamicListMatrix(&outputs);
}

void trainWithoutStopOverfitting(NeuralNet *net, float *init_MSE, float *end_MSE, float *min_MSE, unsigned int *n_epoch_completed, Matrix input, Matrix output, unsigned int n_epochs, float lr) {
    Matrix deriv_mse, deriv_act_func, dC_dw;
    dynamicListMatrix outputs;
    float aux_MSE;
    
    newDynamicListMatrix(&outputs);
    if (getNumberLayers(*net) == 2) {
        Matrix delta;
        Matrix outputNet;
        Layer layer;

        calculatePrediction(&outputs, input, *net);
        consultElemDynamicListMatrix(&outputNet, outputs, 1);
        *init_MSE = MSEMatrix(outputNet, output);
        *min_MSE = *init_MSE;
        
        for (unsigned int i = 0; i < n_epochs; i++) {
            consultElemDynamicListLayer(&layer, net->layers, 0);

            derivMSEMatrix(&deriv_mse, outputNet, output);
            derivActivateFunction(&deriv_act_func, outputNet, layer);
            multiplyNumbersMatrix(&delta, deriv_mse, deriv_act_func);
            
            optimizeBias(&layer, delta, lr);
            transposeMatrix(&delta);
            multiplyMatrix(&dC_dw, delta, input);
            optimizeWeights(&layer, dC_dw, lr);

            changeElemDynamicListLayer(&net->layers, 0, layer);
            
            freeDynamicListMatrix(&outputs);
            calculatePrediction(&outputs, input, *net);
            consultElemDynamicListMatrix(&outputNet, outputs, 1);
            aux_MSE = MSEMatrix(outputNet, output);
            if (aux_MSE < *min_MSE) {
                *min_MSE = aux_MSE;
            }
        }
        consultElemDynamicListMatrix(&outputNet, outputs, 1);
        *end_MSE = MSEMatrix(outputNet, output);
    }
    else { // > 2
        Matrix delta_1, delta_2, aux_w, aux;
        Matrix outputNet;
        Layer current_layer, previous_layer;

        calculatePrediction(&outputs, input, *net);
        consultElemDynamicListMatrix(&outputNet, outputs, net->n_layers - 1);
        *init_MSE = MSEMatrix(outputNet, output);
        *min_MSE = *init_MSE;
        for (int i = 0; i < n_epochs; i++) {

            // Backpropagation and gradient descendent (Train)
            consultElemDynamicListLayer(&current_layer, net->layers, net->n_layers - 2);

            derivMSEMatrix(&deriv_mse, outputNet, output);
            derivActivateFunction(&deriv_act_func, outputNet, current_layer);
            multiplyNumbersMatrix(&delta_1, deriv_mse, deriv_act_func);
            for (int j = net->n_layers - 2; j > 0; j--) {
                consultElemDynamicListMatrix(&outputNet, outputs, j);
                consultElemDynamicListLayer(&current_layer, net->layers, j);
                consultElemDynamicListLayer(&previous_layer, net->layers, j - 1);

                getWeights(&aux_w, current_layer);
                transposeMatrix(&aux_w);

                derivActivateFunction(&deriv_act_func, outputNet, previous_layer);
                multiplyMatrix(&aux, delta_1, aux_w);
                multiplyNumbersMatrix(&delta_2, aux, deriv_act_func);

                transposeMatrix(&outputNet);
                multiplyMatrix(&dC_dw, outputNet, delta_1);
                optimizeWeights(&current_layer, dC_dw, lr);
                optimizeBias(&current_layer, delta_1, lr);

                changeElemDynamicListLayer(&net->layers, j, current_layer);

                delta_1 = delta_2;
            }
            consultElemDynamicListMatrix(&outputNet, outputs, 0);
            consultElemDynamicListLayer(&current_layer, net->layers, 0);

            transposeMatrix(&outputNet);
            multiplyMatrix(&dC_dw, outputNet, delta_1);
            optimizeWeights(&current_layer, dC_dw, lr);
            optimizeBias(&current_layer, delta_1, lr);

            changeElemDynamicListLayer(&net->layers, 0, current_layer);

            // Calculate the error
            freeDynamicListMatrix(&outputs);
            calculatePrediction(&outputs, input, *net);
            consultElemDynamicListMatrix(&outputNet, outputs, net->n_layers - 1);
            aux_MSE = MSEMatrix(outputNet, output);
            if (aux_MSE < *min_MSE) {
                *min_MSE = aux_MSE;
            }
        }
        consultElemDynamicListMatrix(&outputNet, outputs, net->n_layers - 1);
        *end_MSE = MSEMatrix(outputNet, output);
    }
    *n_epoch_completed = n_epochs;
    freeDynamicListMatrix(&outputs);
}

/**
 * FUNCTION: trainNeuralNet
 * INPUT:
 *      A NeuralNet, which obiously has had to be created.
 *      The first matrix is the input matrix.
 *      The second matrix is the output matrix.
 *      Clarification:
 *          The input matrix whose dimensions are (MxN),
 *          M, rows, is the number of data tuples and
 *          N, columns, is the number of values (floats) in a tuple so
 *          if there are 3 neurons in input layer, N = 3.
 *          Example: There are 2 neurons in the input layer and
 *          there are 5 data tuples, so the input matrix is:
 *              2.453 1.784 (Fist tuple)
 *              -4.12 6.438 (Second tuple)
 *              1.546 -3.65 (...)
 *              0.236 -0.836 (...)
 *              1.345 6.367 (M tuple)
 *              
 *              M = 5 and N = 2
 * 
 *          The output matrix whose dimensions are (MxH),
 *          M, rows, is the number of data tuples and
 *          H, columns, is the number of values (floats) in a
 *          tuple so if there 3 neurons in output layer, N = 3.
 *          Example: There are 1 neuron in the output layer and there are
 *          3 data tuples, so the output matrix is:
 *              -1.456
 *              0.657
 *              2.125
 *              
 *              M = 3 and H = 1
 *      
 *      The number of epochs (unsigned int)
 *      The learning rate (float) (Recommended: [0.5, 0.0001]. For example: 0.0025))
 *      The stop overfitting (bool).
 * REQUIREMENTS: 
 *      0 < learning rate <= 1
 *      number of rows (input) > 10
 *      number of rows (input) = number of rows (output)
 * OUTPUT:
 *      initMSE (float).
 *      endMSE (float).
 *      minMSE (float).
 *      number of epochs completed (unsigned int).
 * MODIFIES: The neural network.
 */
void trainNeuralNet(NeuralNet *net, float *init_MSE, float *end_MSE, float *min_MSE, unsigned *n_epoch_completed, Matrix input, Matrix output, unsigned int n_epochs, float lr, bool stop_overfitting) {
    if (numberRows(input) <= 10) {
        errorNeuralNet("The number of rows of the input matrix <= 10.");
    }
    else if (lr <= 0 || lr > 1) {
        errorNeuralNet("The learning rate is out of range.");
    }
    else if (numberRows(input) != numberRows(output)) {
        errorNeuralNet("The number of rows in the input matrix and the output matrix isn't the same.");
    }

    printf("Training...\n");

    if (!stop_overfitting) {
        trainWithoutStopOverfitting(net, init_MSE, end_MSE, min_MSE, n_epoch_completed, input, output, n_epochs, lr);
    }
    else { // Train without stop overfitting
        int n_train_data;
        Matrix aux_input, aux_output;
        Matrix input_train, input_not_train;
        Matrix output_train, output_not_train;
        Matrix decisions;
        
        n_train_data = (int) (OVERFITTING * numberRows(input));
        if (n_train_data == 0) {
            n_train_data = 1;
        }

        aux_input = input;
        aux_output = output;
        suffleRowsMatrix(&aux_input, &decisions);
        sortRowsMatrix(&aux_output, decisions); // Suffle the rows of the matrix like aux_input.
        cutMatrix(&input_train, &input_not_train, aux_input, n_train_data);
        cutMatrix(&output_train, &output_not_train, aux_output, n_train_data);

        trainWithStopOverfitting(net, init_MSE, end_MSE, min_MSE, n_epoch_completed, input_train, input_not_train, output_train, output_not_train, n_epochs, lr);
    }
}

/**
 * FUNCTION: predict
 * INPUT: A input matrix and a neural network.
 * REQUIREMENTS: Obiously the input matrix and the neural
 *      network have had to be created. And the number of
 *      columns of the input matrix must be equal to the number of 
 *      neurons in the input layer.
 * OUTPUT: The output matrix.
 */
void predict(Matrix *out, Matrix input, NeuralNet net) {
    if (numberColumns(input) != (unsigned short) (getNumberInputNeurons(net))) {
        errorNeuralNet("The number of columns of the input matrix and the number of neurons in the input layer isn't the same.");
    }

    dynamicListMatrix outputs;

    newDynamicListMatrix(&outputs);

    calculatePrediction(&outputs, input, net);
    consultElemDynamicListMatrix(out, outputs, getNumberLayers(net) - 1);
    freeDynamicListMatrix(&outputs);
}

/**
 * FUNCTION: getLayers
 * INPUT: A neural network.
 * REQUIREMENTS: Obviously the neural network has to exist.
 * OUTPUT: The array of the numbers of neurons per layer (netStructure).
 */
void getLayers(unsigned char layers[], NeuralNet net) {
    Layer layer;
    int i;
    
    for (i = 0; i < getNumberLayers(net) - 1; i++) {
        consultElemDynamicListLayer(&layer, net.layers, i);
        layers[i] = getNumberNeuronsPreviousLayer(layer);
    }
    consultElemDynamicListLayer(&layer, net.layers, i - 1);
    layers[i] = getNumberNeuronsLayer(layer);
}

/**
 * FUNCTION: getNumberLayers
 * INPUT: A neural network.
 * REQUIREMENTS: Obviously the neural network has to exist.
 * OUTPUT: The number of layers in the neural network.
 */
unsigned char getNumberLayers(NeuralNet net) {
    return net.n_layers;
}

/**
 * FUNCTION: getNumberInputNeurons
 * INPUT: A neural network.
 * REQUIREMENTS: Obviously the neural network has to exist.
 * OUTPUT: The number of inputs neurons in the neural network.
 */
unsigned char getNumberInputNeurons(NeuralNet net) {
    return net.n_inputs;
}

/**
 * FUNCTION: getNumberOutputNeurons
 * INPUT: A neural network.
 * REQUIREMENTS: Obviously the neural network has to exist.
 * OUTPUT: The number of output neurons in the neural network.
 */
unsigned char getNumberOutputNeurons(NeuralNet net) {
    return net.n_outputs;
}

/**
 * FUNCTION: getDescriptionNeuralNet
 * INPUT: A neural network.
 * REQUIREMENTS: Obviously the neural network has to exist.
 * OUTPUT: The description of the neural network.
 */
void getDescriptionNeuralNet(char desc[MAX_DESCRIPTION], NeuralNet net) {
    strcpy(desc, net.description);
}

/**
 * FUNCTION: freeNeuralNetwork
 * INPUT: net (NeuralNet)
 * REQUIREMENTS: None.
 * MODIFIES: The memory of the neural network is released.
 */
void freeNeuralNetwork(NeuralNet net) {
    freeDynamicListLayer(&net.layers);
}

/**
 * FUNCTION: saveNeuralNet
 * INPUT: A neural network and a path.
 *      Example of path:
 *          C:\Users\User\Desktop\net1.aic
 * REQUIREMENTS: Obiously a neural network and a path created.
 * OUTPUT: Save the neural network in the path and the boolean is
 *      the error. Error <=> true
 */
bool saveNeuralNet(NeuralNet net, char path[]) {
    FILE *f;

    if (path[strlen(path) - 1] != 'c' || path[strlen(path) - 2] != 'i' || path[strlen(path) - 3] != 'a' || path[strlen(path) - 4] != '.') {
        printf("Error, invalid extension. It must be .aic\n");
        return true;
    }
    
    f = fopen(path, "wb");
    if (f == NULL) {
        printf("Invalid path.\n");
        return true;
    }

    float a, b, c, d;
    a = (float) (net.n_layers);
    b = (float) (net.n_inputs);
    c = (float) (net.n_outputs);
    d = (float) (strlen(net.description));

    if (fwrite(&a, sizeof(float), 1, f) != 1 || fwrite(&b, sizeof(float), 1, f) != 1 || fwrite(&c, sizeof(float), 1, f) != 1 || fwrite(&d, sizeof(float), 1, f) != 1) {
        printf("Error, the neural network cannot be saved.\n");
        fclose(f);
        return true;
    }

    float e;
    int i, pcent;

    i = 0;
    pcent = strlen(net.description);
    while (i != pcent) {
        e = (float) (net.description[i]);
        if (fwrite(&e, sizeof(float), 1, f) != 1) {
            pcent = i;
        }
        else {
            i++;
        }
    }

    if (pcent != strlen(net.description)) {
        printf("Error, the neural network cannot be saved.\n");
        fclose(f);
        return true;
    }
    else {
        Layer layer;
        bool error;

        i = 0;
        error = false;
        while (i < getNumberLayers(net) - 1 && !error) {
            consultElemDynamicListLayer(&layer, net.layers, i);
            writeLayer(f, &error, layer);
            i++;
        }

        if (error) {
            printf("Error, the neural network cannot be saved.\n");
            fclose(f);
            return true;
        }
        else {
            if (fclose(f) == EOF) {
                printf("The neural network can't be saved.\n");
                return true;
            }
            else {
                printf("Neural network saved.\n");
                return false;
            }
        }
    }
}

/**
 * FUNCTION: openNeuralNet
 * INPUT: A path of a previosly saved neural network.
 *      Example of path:
 *          C:\Users\User\Desktop\net1.aic
 * REQUIREMENTS: Obiously the file has to exits.
 * OUTPUT: Open the neural network in NeuralNetwork and
 *      the boolean is the error. Error <=> True.
 */
bool openNeuralNet(NeuralNet *net, char path[]) {
    FILE *f;
    
    f = fopen(path, "rb");
    if (f == NULL) {
        printf("Invalid path.\n");
        return true;
    }

    if (feof(f)) {
        printf("Empty file.\n");
        return true;
    }

    float a, b, c, length_desc, e;
    if (fread(&a, sizeof(float), 1, f) != 1 || fread(&b, sizeof(float), 1, f) != 1 || fread(&c, sizeof(float), 1, f) != 1 || fread(&length_desc, sizeof(float), 1, f) != 1) {
        printf("Error, the file cannot be read.\n");
        return true;
    }

    net->n_layers = (unsigned char) (a);
    net->n_inputs = (unsigned char) (b);
    net->n_outputs = (unsigned char) (c);
    
    int i = 0;
    while (i < length_desc && fread(&e, sizeof(float), 1, f) == 1) {
        net->description[i] = (char) (e);
        i++;
    }
    net->description[i] = '\0';
    
    if (i < length_desc) {
        printf("Error, the file cannot be read.\n");
        return true;
    }
    else {
        // Read the layers
        if (feof(f)) {
            printf("Error, the file cannot be read.\n");
            return true;
        }
        else {
            Layer layer;
            bool error;

            newDynamicListLayer(&net->layers);
            
            i = 0;
            error = false;
            while (i < net->n_layers - 1 && !error) {
                readLayer(f, &layer, &error);
                appendDynamicListLayer(&net->layers, layer);
                i++;
            }
            return error;
        }
    }
}