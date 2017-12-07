#include  <iostream>
#include <chrono>
#include "../../src/npycrf/sampler.h"
#include "../../src/npycrf/ctype.h"
#include "../../src/python/model.h"
#include "../../src/python/dataset.h"
#include "../../src/python/dictionary.h"
#include "../../src/python/trainer.h"
#include "../../src/python/model/crf.h"
#include "../../src/python/model/npylm.h"
using namespace npycrf;
using namespace npycrf::python;
using std::cout;
using std::flush;
using std::endl;

void run_training_loop(){
	std::string filename = "../../dataset/test.txt";
	Corpus* corpus_u = new Corpus();
	corpus_u->add_textfile(filename);
	
	Corpus* corpus_l = new Corpus();
	std::vector<std::wstring> word_str_vec = {L"iii", L"iiiqqq", L"nnnvvv", L"pppyyy", L"ppp", L"pppnnn", L"vvvfff", L"uuubbb", L"bbbhhh", L"bbbiii", L"aaaxxx", L"yyyppp", L"jjjqqq", L"tttiii"};
	corpus_l->add_true_segmentation(word_str_vec);

	Dictionary* dict = new Dictionary();

	int seed = 0;
	Dataset* dataset_l = new Dataset(corpus_l, dict, 1, seed);
	Dataset* dataset_u = new Dataset(corpus_u, dict, 1, seed);

	double lambda_0 = 1;
	int max_word_length = 12;
	int max_sentence_length = std::max(dataset_l->get_max_sentence_length(), dataset_u->get_max_sentence_length());
	double g0 = 1.0 / (double)dict->get_num_characters();
	double initial_lambda_a = 4;
	double initial_lambda_b = 1;
	double vpylm_beta_stop = 4;
	double vpylm_beta_pass = 1;
	model::NPYLM* py_npylm = new model::NPYLM(max_word_length, max_sentence_length, g0, initial_lambda_a, initial_lambda_b, vpylm_beta_stop, vpylm_beta_pass);

	int num_character_ids = dict->get_num_characters();
	int num_character_types = CTYPE_NUM_TYPES;
	int feature_x_unigram_start = -2;
	int feature_x_unigram_end = 2;
	int feature_x_bigram_start = -2;
	int feature_x_bigram_end = 1;
	int feature_x_identical_1_start = -2;
	int feature_x_identical_1_end = 1;
	int feature_x_identical_2_start = -3;
	int feature_x_identical_2_end = 1;
	double sigma = 1;
	model::CRF* py_crf = new model::CRF(num_character_ids,
										num_character_types,
										feature_x_unigram_start,
										feature_x_unigram_end,
										feature_x_bigram_start,
										feature_x_bigram_end,
										feature_x_identical_1_start,
										feature_x_identical_1_end,
										feature_x_identical_2_start,
										feature_x_identical_2_end,
										sigma);

	Model* model = new Model(py_npylm, py_crf, lambda_0);
	dict->save("npylm.dict");
	double learning_rate = 0.001;
	unsigned int batchsize = 32;
	double crf_regularization_constant = 1;

	Trainer* trainer = new Trainer(dataset_l, dataset_u, dict, model, crf_regularization_constant);
	trainer->add_labelded_data_to_npylm();
	trainer->sgd(learning_rate, batchsize, true);
	for(int epoch = 1;epoch < 200;epoch++){
		auto start_time = std::chrono::system_clock::now();
		trainer->gibbs();
		trainer->sgd(learning_rate, batchsize, false);
	    auto diff = std::chrono::system_clock::now() - start_time;
	    cout << (std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() / 1000.0) << endl;
		trainer->sample_hpylm_vpylm_hyperparameters();
		trainer->sample_npylm_lambda();
		if(epoch > 3){
			trainer->update_p_k_given_vpylm();
		}
		if(epoch % 10 == 0){
			trainer->print_segmentation_train(10);
			cout << "ppl: " << trainer->compute_perplexity_train() << endl;
			trainer->print_segmentation_dev(10);
			cout << "ppl: " << trainer->compute_perplexity_dev() << endl;
			cout << "log_likelihood: " << trainer->compute_log_likelihood_train() << endl;
			cout << "log_likelihood: " << trainer->compute_log_likelihood_dev() << endl;
		}
	}
	delete dict;
	delete dataset_l;
	delete dataset_u;
	delete trainer;
	delete model;
}

int main(int argc, char *argv[]){
	for(int i = 0;i < 10;i++){
		run_training_loop();
	}
}