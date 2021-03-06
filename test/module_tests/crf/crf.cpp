#include <unordered_map>
#include <iostream>
#include <cassert>
#include "../../../src/npycrf/sentence.h"
#include "../../../src/npycrf/crf/crf.h"

using namespace npycrf::crf;
using namespace npycrf;
using std::cout;
using std::flush;
using std::endl;

void test_init(){
	int num_character_ids = 100;
	int num_character_types = 281;
	int feature_x_unigram_start = -2;
	int feature_x_unigram_end = 2;
	int feature_x_bigram_start = -2;
	int feature_x_bigram_end = 1;
	int feature_x_identical_1_start = -2;
	int feature_x_identical_1_end = 1;
	int feature_x_identical_2_start = -3;
	int feature_x_identical_2_end = 1;
	double sigma = 1;
	CRF* crf = new CRF(num_character_ids,
					   num_character_types,
					   feature_x_unigram_start,
					   feature_x_unigram_end,
					   feature_x_bigram_start,
					   feature_x_bigram_end,
					   feature_x_identical_1_start,
					   feature_x_identical_1_end,
					   feature_x_identical_2_start,
					   feature_x_identical_2_end,
					   sigma
	);

	double value = 1.5;
	for(int y_i = 0;y_i < 2;y_i++){
		for(int x_i = 0;x_i < num_character_ids;x_i++){
			for(int i = 1;i <= feature_x_unigram_end - feature_x_unigram_start + 1;i++){
				crf->set_w_unigram_u(y_i, i, x_i, value);
				for(int y_i_1 = 0;y_i_1 < 2;y_i_1++){
					crf->set_w_unigram_b(y_i_1, y_i, i, x_i, value);
				}
			}
			for(int i = 1;i <= feature_x_bigram_end - feature_x_bigram_start + 1;i++){
				for(int x_i_1 = 0;x_i_1 < num_character_ids;x_i_1++){
					crf->set_w_bigram_u(y_i, i, x_i_1, x_i, value);
					for(int y_i_1 = 0;y_i_1 < 2;y_i_1++){
						crf->set_w_bigram_b(y_i_1, y_i, i, x_i_1, x_i, value);
					}
				}
			}
			
		}
		for(int i = 1;i <= feature_x_identical_1_end - feature_x_identical_1_start + 1;i++){
			crf->set_w_identical_1_u(y_i, i, value);
			for(int y_i_1 = 0;y_i_1 < 2;y_i_1++){
				crf->set_w_identical_1_b(y_i_1, y_i, i, value);
			}
		}
		for(int i = 1;i <= feature_x_identical_2_end - feature_x_identical_2_start + 1;i++){
			crf->set_w_identical_2_u(y_i, i, value);
			for(int y_i_1 = 0;y_i_1 < 2;y_i_1++){
				crf->set_w_identical_2_b(y_i_1, y_i, i, value);
			}
		}
		for(int type_i = 0;type_i < num_character_types;type_i++){
			crf->set_w_unigram_type_u(y_i, type_i, value);
			for(int y_i_1 = 0;y_i_1 < 2;y_i_1++){
				crf->set_w_unigram_type_b(y_i_1, y_i, type_i, value);
			}
			for(int type_i_1 = 0;type_i_1 < num_character_types;type_i_1++){
				crf->set_w_bigram_type_u(y_i, type_i_1, type_i, value);
				for(int y_i_1 = 0;y_i_1 < 2;y_i_1++){
					crf->set_w_bigram_type_b(y_i_1, y_i, type_i_1, type_i, value);
				}
			}
		}
	}

	for(int i = 0;i < crf->_w_size_unigram_u + crf->_w_size_unigram_b;i++){
		assert(crf->_w_unigram[i] == value);
	}
	for(int i = 0;i < crf->_w_size_bigram_u + crf->_w_size_bigram_b;i++){
		assert(crf->_w_bigram[i] == value);
	}
	for(int i = 0;i < crf->_w_size_identical_1_u + crf->_w_size_identical_1_b;i++){
		assert(crf->_w_identical_1[i] == value);
	}
	for(int i = 0;i < crf->_w_size_identical_2_u + crf->_w_size_identical_2_b;i++){
		assert(crf->_w_identical_2[i] == value);
	}
	for(int i = 0;i < crf->_w_size_unigram_type_u + crf->_w_size_unigram_type_b;i++){
		assert(crf->_w_unigram_type[i] == value);
	}
	for(int i = 0;i < crf->_w_size_bigram_type_u + crf->_w_size_bigram_type_b;i++){
		assert(crf->_w_bigram_type[i] == value);
	}
	delete crf;
}

void test_compute_path_cost(){
	std::unordered_map<wchar_t, int> _token_ids;
	std::wstring sentence_str = L"ああいうえええおかきくくけこさささしすせそ";
	for(wchar_t character: sentence_str){
		auto itr = _token_ids.find(character);
		if(itr == _token_ids.end()){
			_token_ids[character] = _token_ids.size();
		}
	}

	int num_character_ids = _token_ids.size();
	int num_character_types = 281;
	int feature_x_unigram_start = -2;
	int feature_x_unigram_end = 2;
	int feature_x_bigram_start = -2;
	int feature_x_bigram_end = 1;
	int feature_x_identical_1_start = -2;
	int feature_x_identical_1_end = 1;
	int feature_x_identical_2_start = -3;
	int feature_x_identical_2_end = 2;
	double sigma = 1;
	CRF* crf = new CRF(num_character_ids,
					   num_character_types,
					   feature_x_unigram_start,
					   feature_x_unigram_end,
					   feature_x_bigram_start,
					   feature_x_bigram_end,
					   feature_x_identical_1_start,
					   feature_x_identical_1_end,
					   feature_x_identical_2_start,
					   feature_x_identical_2_end,
					   sigma
	);
	int* character_ids = new int[sentence_str.size()];
	for(int i = 0;i < sentence_str.size();i++){
		character_ids[i] = _token_ids[sentence_str[i]];
	}
	for(int i = 1;i <= crf->_x_range_unigram;i++){
		for(int x_i = 0;x_i < _token_ids.size();x_i++){
			crf->set_w_unigram_u(0, i, x_i, x_i);
			crf->set_w_unigram_b(0, 0, i, x_i, x_i * 2);
		}
	}
	for(int i = 1;i <= sentence_str.size();i++){
		double cost = crf->_compute_cost_unigram_features(character_ids, sentence_str.size(), i, 0, 0);
		double true_cost = 0;
		for(int t = std::max(0, std::min((int)sentence_str.size(), i - crf->_x_range_unigram));t < i;t++){
			true_cost += character_ids[t] * 3;
		}
		assert(cost == true_cost);
	}
	for(int i = 1;i <= crf->_x_range_bigram;i++){
		for(int x_i = 0;x_i < _token_ids.size();x_i++){
			for(int x_i_1 = 0;x_i_1 < _token_ids.size();x_i_1++){
				crf->set_w_bigram_u(0, i, x_i_1, x_i, x_i * x_i_1);
				crf->set_w_bigram_b(0, 0, i, x_i_1, x_i, x_i * x_i_1 * 2);
			}
		}
	}
	for(int i = 2;i <= sentence_str.size();i++){
		double cost = crf->_compute_cost_bigram_features(character_ids, sentence_str.size(), i, 0, 0);
		double true_cost = 0;
		for(int t = std::max(1, std::min((int)sentence_str.size(), i - crf->_x_range_bigram));t < i;t++){
			true_cost += character_ids[t] * character_ids[t - 1] * 3;
		}
		assert(cost == true_cost);
	}
	for(int i = 1;i <= crf->_x_range_identical_1;i++){
		crf->set_w_identical_1_u(0, i, i);
		crf->set_w_identical_1_b(0, 0, i, i * 2);
	}
	for(int i = 1;i <= sentence_str.size();i++){
		double cost = crf->_compute_cost_identical_1_features(character_ids, sentence_str.size(), i, 0, 0);
		if(i == 1){
			assert(cost == 0);
			continue;
		}
		double true_cost = 0;
		for(int r = i;r >= std::max(2, i - crf->_x_range_identical_1 + 1);r--){
			if(character_ids[r - 1] == character_ids[r - 2]){
				true_cost += (i - r + 1) * 3;
			}
		}
		assert(cost == true_cost);
	}
	for(int i = 1;i <= crf->_x_range_identical_2;i++){
		crf->set_w_identical_2_u(0, i, i);
		crf->set_w_identical_2_b(0, 0, i, i * 2);
	}
	for(int i = 1;i <= sentence_str.size();i++){
		double cost = crf->_compute_cost_identical_2_features(character_ids, sentence_str.size(), i, 0, 0);
		if(i < 3){
			assert(cost == 0);
			continue;
		}
		double true_cost = 0;
		for(int r = i;r >= std::max(3, i - crf->_x_range_identical_2 + 1);r--){
			if(character_ids[r - 1] == character_ids[r - 3]){
				true_cost += (i - r + 1) * 3;
			}
		}
		assert(cost == true_cost);
	}
	
	delete crf;
	delete[] character_ids;
}

void test_grads(){
	std::unordered_map<wchar_t, int> _token_ids;
	std::wstring sentence_str = L"あああいいいいうううううええええおおお";
	for(wchar_t character: sentence_str){
		auto itr = _token_ids.find(character);
		if(itr == _token_ids.end()){
			_token_ids[character] = _token_ids.size();
		}
	}

	int num_character_ids = _token_ids.size();
	int num_character_types = 281;
	int feature_x_unigram_start = -2;
	int feature_x_unigram_end = 2;
	int feature_x_bigram_start = -2;
	int feature_x_bigram_end = 1;
	int feature_x_identical_1_start = -2;
	int feature_x_identical_1_end = 1;
	int feature_x_identical_2_start = -3;
	int feature_x_identical_2_end = 2;
	double sigma = 1;
	CRF* crf = new CRF(num_character_ids,
					   num_character_types,
					   feature_x_unigram_start,
					   feature_x_unigram_end,
					   feature_x_bigram_start,
					   feature_x_bigram_end,
					   feature_x_identical_1_start,
					   feature_x_identical_1_end,
					   feature_x_identical_2_start,
					   feature_x_identical_2_end,
					   sigma
	);
	int* character_ids = new int[sentence_str.size()];
	for(int i = 0;i < sentence_str.size();i++){
		character_ids[i] = _token_ids[sentence_str[i]];
	}
	Sentence* sentence = new Sentence(sentence_str, character_ids);
	std::vector<int> segments {3, 4, 5, 4, 3};
	sentence->split(segments);
	sentence->dump_words();

	delete crf;
	delete[] character_ids;
}

int main(){
	setlocale(LC_CTYPE, "ja_JP.UTF-8");
	std::ios_base::sync_with_stdio(false);
	std::locale default_loc("ja_JP.UTF-8");
	std::locale::global(default_loc);
	std::locale ctype_default(std::locale::classic(), default_loc, std::locale::ctype); //※
	std::wcout.imbue(ctype_default);
	std::wcin.imbue(ctype_default);
	// test_grads();
	// cout << "OK" << endl;
	test_init();
	cout << "OK" << endl;
	// test_compute_path_cost();
	// cout << "OK" << endl;
	return 0;
}