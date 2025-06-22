/*

    PQC-MATH-SIM.CPP            @author: Mayank Pandey                @date: 06/11/2025
    This file is part of the PQC-MATH-SIM project, which simulates a weakened system of
    equations for the Learning With Errors (LWE) post-quantum cryptography (PQC) scheme
    using C++.

    Formula: A * s + e = b mod q (optional + 1/2)'s the formula for this program, where
            A = coefficients         e = c++ rng generated
            s = alice_private_key    b = rhs
            q = modulus layer
        
    Features are everything except the label actual_bit.

*/

#include <fstream>
#include <numeric>
#include <random> 
#include <iostream>
#include <vector>
using namespace std;

// Correct modulus 
int positive_modulus(int a, int m) {
    return (a % m + m) % m;
}

int RNGseed1 = 42   +1;
int RNGseed2 = 1337 +1;//Originals didn't have +1
mt19937 rng(RNGseed1);

vector<int> public_data(int bit) {
    // Parameters for the system of equations
    struct SysEq {
        vector<int> alice_private_key =  {7, 11, 3, 14}; // Original Key: {10, 13, 8, 5}; // ; // 
        int coeff_range = 3;
        int num_eqs = 20;
        int roughly_zero = 1;
        int modulus_layer = 36;
        int equation_selection_amount = 14;
        int training_data_instances = 4;
    };

    // Variable initializations
    SysEq sysEq;
    vector<vector<int>> coefficients(sysEq.num_eqs, vector<int>(sysEq.alice_private_key.size()));
    vector<int> rhs(sysEq.num_eqs);
    // random_device dev;
    uniform_int_distribution<mt19937::result_type> distcoeff(-sysEq.coeff_range, sysEq.coeff_range);
    uniform_int_distribution<mt19937::result_type> roughlyzero(-sysEq.roughly_zero, sysEq.roughly_zero);
    
    // Generate the system of equations
    for (int i = 0; i < sysEq.num_eqs; ++i) {
        int sum = 0;
        int total_error = 0;
        for (int j = 0; j < sysEq.alice_private_key.size(); ++j) {
            int coeff = distcoeff(rng);
            coefficients[i][j] = coeff;
            sum += coeff * sysEq.alice_private_key[j];
        }
        rhs[i] = positive_modulus(sum + roughlyzero(rng), sysEq.modulus_layer);
    }
    
    // Initialize RNGs for bit encryption
    uniform_int_distribution<mt19937::result_type> equ_selector(0, sysEq.num_eqs - 1);
    
    // Individual bit encryption
    vector<vector<int>> equation_lhs(
        sysEq.equation_selection_amount, vector<int>(sysEq.alice_private_key.size(), 0)
    );
    vector<int> equation_rhs(
        sysEq.equation_selection_amount
    );
    vector<int> r(sysEq.num_eqs);

    // Intermediate summations
    for (int equation_number = 0; equation_number < sysEq.equation_selection_amount; equation_number++) {
        int selected_equation = equ_selector(rng);
        r[selected_equation] += 1;
        int current_lhs = 0;
        int current_rhs = rhs[selected_equation];
        for (int coeff_pointer = 0; coeff_pointer < sysEq.alice_private_key.size(); coeff_pointer++) {
            equation_lhs[equation_number][coeff_pointer] = coefficients[selected_equation][coeff_pointer];
        }
        equation_rhs[equation_number] = rhs[selected_equation];
    }

    // Summation finalizations
    int sum_rhs = positive_modulus(
        accumulate(equation_rhs.begin(), equation_rhs.end(), 0), sysEq.modulus_layer
    );
    vector<int> coeffs_lhs(
        sysEq.alice_private_key.size(), 0
    );

    for (int equation_append = 0; equation_append < equation_lhs.size(); equation_append++) {
        for (int equation_append_coeff = 0; equation_append_coeff < sysEq.alice_private_key.size(); equation_append_coeff++) {
            coeffs_lhs[equation_append_coeff] += equation_lhs[equation_append][equation_append_coeff];
        }
    }
    
    int sum_lhs = 0;
    for (int i = 0; i < coeffs_lhs.size(); ++i) {
        sum_lhs += coeffs_lhs[i]  * sysEq.alice_private_key[i];
    }
    sum_lhs = positive_modulus(sum_lhs + (bit * (sysEq.modulus_layer/2)), sysEq.modulus_layer);

    // Flattenning coeffs_lhs and sum_rhs
    vector<int> flatten(sysEq.alice_private_key.size() + 1);
    int flatten_counter = 0;
    for (int coeff_lhs : coeffs_lhs) {
        flatten[flatten_counter] = coeff_lhs;
        flatten_counter += 1;
    }
    flatten[flatten_counter] = sum_rhs;
    return flatten;
}

int main() {
    // Some more user-adjustable parameters :p
    int csv_rows = 10000;

    // Dataset initialization and production
    ofstream file_out("default.mod-rng-and-key.csv");
    random_device dev_bit;
    mt19937 rng_bit(RNGseed2);
    uniform_int_distribution<mt19937::result_type> random_bit(0, 1);

    file_out << "coefficient_1,coefficient_2,coefficient_3,coefficient_4,rhs_bit,actual_bit" << endl;
    for (int csv_row = 0; csv_row < csv_rows; csv_row++) {
        int current_bit = random_bit(rng_bit);
        for (int value : public_data(current_bit)) {
            file_out << value << ",";
        }
        file_out << current_bit << endl;
    }
}

// Run:
// clear && g++ pqc-math-sim.cpp && ./a.out && rm -rfv a.out
// Need:
// Normal, RNG-diff, key-diff, key+RNG-diff (1 dataset only)