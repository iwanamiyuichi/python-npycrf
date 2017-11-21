#include <algorithm>
#include <iostream>
#include "hash.h"
#include "sampler.h"
#include "lattice.h"

// ＿人人人人人人人人人人人人人人人人人人人人人人人人人人人人＿
// ＞　Latticeでは文字のインデックスtが1から始まることに注意　＜
// ￣Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y^Y￣

namespace npycrf {
	using namespace npylm;
	namespace lattice {
		template<typename T>
		void _init_array(T* &array, int size_i){
			array = new T[size_i];
			assert(array != NULL);
		}
		template<typename T>
		void _init_array(T** &array, int size_i, int size_j){
			array = new T*[size_i];
			assert(array != NULL);
			for(int i = 0;i < size_i;i++){
				array[i] = new T[size_j];
				assert(array[i] != NULL);
			}
		}
		template<typename T>
		void _init_array(T*** &array, int size_i, int size_j, int size_k){
			array = new T**[size_i];
			assert(array != NULL);
			for(int i = 0;i < size_i;i++){
				array[i] = new T*[size_j];
				assert(array[i] != NULL);
				for(int j = 0;j < size_j;j++){
					array[i][j] = new T[size_k];
					assert(array[i][j] != NULL);
				}
			}
		}
		template<typename T>
		void _init_array(T**** &array, int size_i, int size_j, int size_k, int size_l){
			array = new T***[size_i];
			assert(array != NULL);
			for(int i = 0;i < size_i;i++){
				array[i] = new T**[size_j];
				assert(array[i] != NULL);
				for(int j = 0;j < size_j;j++){
					array[i][j] = new T*[size_k];
					assert(array[i][j] != NULL);
					for(int k = 0;k < size_k;k++){
						array[i][j][k] = new T[size_l];
						assert(array[i][j][k] != NULL);
					}
				}
			}
		}
		template<typename T>
		void _delete_array(T* &array, int size_i){
			if(array == NULL){
				return;
			}
			delete[] array;
		}
		template<typename T>
		void _delete_array(T** &array, int size_i, int size_j){
			if(array == NULL){
				return;
			}
			for(int i = 0;i < size_i;i++){
				delete[] array[i];
			}
			delete[] array;
		}
		template<typename T>
		void _delete_array(T*** &array, int size_i, int size_j, int size_k){
			if(array == NULL){
				return;
			}
			for(int i = 0;i < size_i;i++){
				for(int j = 0;j < size_j;j++){
					delete[] array[i][j];
				}
				delete[] array[i];
			}
			delete[] array;
		}
		template<typename T>
		void _delete_array(T**** &array, int size_i, int size_j, int size_k, int size_l){
			if(array == NULL){
				return;
			}
			for(int i = 0;i < size_i;i++){
				for(int j = 0;j < size_j;j++){
					for(int k = 0;k < size_j;k++){
						delete[] array[i][j][k];
					}
					delete[] array[i][j];
				}
				delete[] array[i];
			}
			delete[] array;
		}
		template<typename T>
		void _init_table(T*** &table, int size, int max_word_length){
			table = new T**[size];
			assert(table != NULL);
			for(int t = 0;t < size;t++){
				table[t] = new T*[max_word_length + 1];
				assert(table[t] != NULL);
				for(int k = 0;k < max_word_length + 1;k++){
					table[t][k] = new T[max_word_length + 1];
					assert(table[t][k] != NULL);
					for(int j = 0;j < max_word_length + 1;j++){
						table[t][k][j] = 0;
					}
				}
			}
		}
		template<typename T>
		void _delete_table(T*** &table, int size, int max_word_length){
			if(table == NULL){
				return;
			}
			for(int t = 0;t < size;t++){
				for(int k = 0;k < max_word_length + 1;k++){
					delete[] table[t][k];
				}
				delete[] table[t];
			}
			delete[] table;
			table = NULL;
		}
	}
	Lattice::Lattice(NPYLM* npylm, crf::CRF* crf, double lambda_0, int max_word_length, int max_sentence_length){
		_npylm = npylm;
		_crf = crf;
		_word_ids = new id[3];	// 3-gram
		_alpha = NULL;
		_beta = NULL;
		_pw_h = NULL;
		_scaling = NULL;
		_pc_s = NULL;
		_backward_sampling_table = NULL;
		_viterbi_backward = NULL;
		_substring_word_id_cache = NULL;
		_lambda_0 = lambda_0;
		_allocate_capacity(max_word_length, max_sentence_length);
	}
	Lattice::~Lattice(){
		delete[] _word_ids;
		_delete_capacity();
	}
	// 必要ならキャッシュの再確保
	void Lattice::reserve(int max_word_length, int max_sentence_length){
		if(max_word_length <= _max_word_length && max_sentence_length <= _max_sentence_length){
			return;
		}
		_delete_capacity();
		_allocate_capacity(max_word_length, max_sentence_length);
		_max_word_length = max_word_length;
		_max_sentence_length = max_sentence_length;
	}
	void Lattice::_allocate_capacity(int max_word_length, int max_sentence_length){
		_max_word_length = max_word_length;
		_max_sentence_length = max_sentence_length;
		// 必要な配列の初期化
		int seq_capacity = max_sentence_length + 1;
		int word_capacity = max_word_length + 1;
		// 前向き確率のスケーリング係数
		lattice::_init_array(_scaling, seq_capacity + 1);
		// ビタビアルゴリズム用
		lattice::_init_array(_viterbi_backward, seq_capacity, word_capacity, word_capacity);
		// 後ろ向きアルゴリズムでkとjをサンプリングするときの確率表
		lattice::_init_array(_backward_sampling_table, word_capacity * word_capacity);
		// 前向き確率
		lattice::_init_array(_alpha, seq_capacity + 1, word_capacity, word_capacity);
		// 後向き確率
		lattice::_init_array(_beta, seq_capacity + 1, word_capacity, word_capacity);
		// 部分文字列が単語になる条件付き確率テーブル
		lattice::_init_array(_pc_s, seq_capacity, word_capacity);
		// 3-gram確率のキャッシュ
		lattice::_init_array(_pw_h, seq_capacity, word_capacity, word_capacity, word_capacity);
		// 部分文字列のIDのキャッシュ
		lattice::_init_array(_substring_word_id_cache, seq_capacity, word_capacity);
	}
	void Lattice::_delete_capacity(){
		int seq_capacity = _max_sentence_length + 1;
		int word_capacity = _max_word_length + 1;
		lattice::_delete_array(_scaling, seq_capacity + 1);
		lattice::_delete_array(_viterbi_backward, seq_capacity, word_capacity, word_capacity);
		lattice::_delete_array(_backward_sampling_table, word_capacity * word_capacity);
		lattice::_delete_array(_alpha, seq_capacity + 1, word_capacity, word_capacity);
		lattice::_delete_array(_beta, seq_capacity + 1, word_capacity, word_capacity);
		lattice::_delete_array(_pc_s, seq_capacity, word_capacity);
		lattice::_delete_array(_pw_h, seq_capacity, word_capacity, word_capacity, word_capacity);
		lattice::_delete_array(_substring_word_id_cache, seq_capacity, word_capacity);
	}
	id Lattice::get_substring_word_id_at_t_k(Sentence* sentence, int t, int k){
		assert(t < _max_sentence_length + 1);
		assert(k < _max_sentence_length + 1);
		assert(t - k >= 0);
		id word_id = _substring_word_id_cache[t][k];
		if(word_id == 0){
			word_id = sentence->get_substr_word_id(t - k, t - 1);	// 引数はインデックスなので注意
			_substring_word_id_cache[t][k] = word_id;
		}
		return word_id;
	}
	// alpha[t-k][j][i]自体は正規化されている場合があるが、alpha[t][k][j]の正規化はここでは行わない
	// @args
	// 	pw_h_tkji;  NPYLMによる確率計算の結果をpw_h_tkjiにキャッシュする
	// NPYLMによる確率計算の結果をpw_h_tkjiにキャッシュする
	// このキャッシュは後向き確率の計算時に使う
	void Lattice::_sum_alpha_t_k_j(Sentence* sentence, int t, int k, int j, double*** alpha, double**** pw_h_tkji, double prod_scaling){
		id word_k_id = get_substring_word_id_at_t_k(sentence, t, k);
		wchar_t const* characters = sentence->_characters;
		int const* character_ids = sentence->_character_ids;
		int character_ids_length = sentence->size();
		assert(t <= _max_sentence_length + 1);
		assert(k <= _max_word_length);
		assert(j <= _max_word_length);
		assert(t - k >= 0);
		// <bos>から生成されている場合
		if(j == 0){
			_word_ids[0] = ID_BOS;
			_word_ids[1] = ID_BOS;
			_word_ids[2] = word_k_id;
			double pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t - k, t - 1);
			assert(pw_h > 0);
			double potential = _crf->compute_trigram_potential(character_ids, characters, character_ids_length, t, k, j);
			double p = exp(_lambda_0 * log(pw_h) + potential);
			assert(p > 0);
			alpha[t][k][0] = p * prod_scaling;
			pw_h_tkji[t][k][0][0] = pw_h;
			return;
		}
		// i=0に相当
		if(t - k - j == 0){
			id word_j_id = get_substring_word_id_at_t_k(sentence, t - k, j);
			_word_ids[0] = ID_BOS;
			_word_ids[1] = word_j_id;
			_word_ids[2] = word_k_id;
			double pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t - k, t - 1);
			assert(pw_h > 0);
			assert(alpha[t - k][j][0] > 0);
			double potential = _crf->compute_trigram_potential(character_ids, characters, character_ids_length, t, k, j);
			double p = exp(_lambda_0 * log(pw_h) + potential);
			assert(p > 0);
			alpha[t][k][j] = p * alpha[t - k][j][0] * prod_scaling;
			assert(alpha[t][k][j] > 0);
			pw_h_tkji[t][k][j][0] = pw_h;
			return;
		}
		// それ以外の場合は周辺化
		double sum = 0;
		for(int i = 1;i <= std::min(t - k - j, _max_word_length);i++){
			id word_i_id = get_substring_word_id_at_t_k(sentence, t - k - j, i);
			id word_j_id = get_substring_word_id_at_t_k(sentence, t - k, j);
			_word_ids[0] = word_i_id;
			_word_ids[1] = word_j_id;
			_word_ids[2] = word_k_id;
			double pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t - k, t - 1);
			assert(pw_h > 0);
			assert(i <= _max_word_length);
			assert(alpha[t - k][j][i] > 0);
			double potential = _crf->compute_trigram_potential(character_ids, characters, character_ids_length, t, k, j);
			double p = exp(_lambda_0 * log(pw_h) + potential);
			assert(p > 0);
			double value = p * alpha[t - k][j][i];

			#ifdef __DEBUG__
				if(value == 0){
					std::cout << pw_h << std::endl;
					std::cout << alpha[t - k][j][i] << std::endl;
					std::cout << t << ", " << k << ", " << j << ", " << i << std::endl;
				}
			#endif

			assert(value > 0);
			sum += value;
			pw_h_tkji[t][k][j][i] = pw_h;
		}
		assert(sum > 0);
		alpha[t][k][j] = sum * prod_scaling;
	}
	void Lattice::forward_filtering(Sentence* sentence, bool use_scaling){
		_enumerate_forward_variables(sentence, _alpha, _pw_h, _scaling, use_scaling);
	}
	void Lattice::backward_sampling(Sentence* sentence, std::vector<int> &segments){
		_backward_sampling(sentence, _alpha, segments);
	}
	// 求めた前向き確率テーブルをもとに後ろから分割をサンプリング
	void Lattice::_backward_sampling(Sentence* sentence, double*** alpha, std::vector<int> &segments){
		segments.clear();
		int k = 0;
		int j = 0;
		int sum = 0;
		int t = sentence->size();
		sample_backward_k_and_j(sentence, t, 1, k, j);
		assert(k <= _max_word_length);
		segments.push_back(k);
		if(j == 0 && k == t){	// 文章すべてが1単語になる場合
			return;
		}
		assert(k > 0 && j > 0);
		assert(j <= _max_word_length);
		segments.push_back(j);
		// std::cout << "<- " << k << std::endl;
		// std::cout << "<- " << j << std::endl;
		t -= k;
		t -= j;
		sum += k + j;
		int next_word_length = j;
		while(t > 0){
			if(t == 1){
				k = 1;
				j = 0;
			}else{
				sample_backward_k_and_j(sentence, t, next_word_length, k, j);
				assert(k > 0);
				assert(k <= _max_word_length);
			}
			segments.push_back(k);
			// std::cout << "<- " << k << std::endl;
			t -= k;
			if(j == 0){
				assert(t == 0);
			}else{
				assert(j <= _max_word_length);
				segments.push_back(j);
				// std::cout << "<- " << j << std::endl;
				t -= j;
			}
			sum += k + j;
			next_word_length = j;
		}
		assert(t == 0);
		assert(sum == sentence->size());
		reverse(segments.begin(), segments.end());
	}
	// 後向きにkとjをサンプリング
	void Lattice::sample_backward_k_and_j(Sentence* sentence, int t, int next_word_length, int &sampled_k, int &sampled_j){
		int table_index = 0;
		wchar_t const* characters = sentence->_characters;
		int character_ids_length = sentence->size();
		double sum_p = 0;
		int limit_k = std::min(t, _max_word_length);
		for(int k = 1;k <= limit_k;k++){
			for(int j = 1;j <= std::min(t - k, _max_word_length);j++){
				id word_j_id = get_substring_word_id_at_t_k(sentence, t - k, j);
				id word_k_id = get_substring_word_id_at_t_k(sentence, t, k);
				id word_t_id = ID_EOS;
				if(t < sentence->size()){
					assert(t + next_word_length <= sentence->size());
					assert(next_word_length > 0);
					word_t_id = get_substring_word_id_at_t_k(sentence, t + next_word_length, next_word_length);
					// id word_id = sentence->get_substr_word_id(t - 1, t + next_word_length - 2);
					// assert(word_t_id == word_id);
				}
				_word_ids[0] = word_j_id;
				_word_ids[1] = word_k_id;
				_word_ids[2] = word_t_id;
				double pw_h = 0;
				if(t == sentence->size()){	// <eos>に接続する確率からサンプリング
					pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t, t);
				}else{
					pw_h = _pw_h[t + next_word_length][next_word_length][k][j];
					#ifdef __DEBUG__
						double pw_h2 = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t, t + next_word_length - 1);
						if(pw_h != pw_h2){
							std::cout << "t = " << t << ", k = " << k << ", j = " << j << std::endl;
							std::cout << "next_word_length = " << next_word_length << std::endl;
							std::cout << "size = " << sentence->size() << std::endl;
						}
						assert(pw_h == pw_h2);
					#endif
				}
				assert(_alpha[t][k][j] > 0);
				double p = pw_h * _alpha[t][k][j];
				assert(p > 0);
				sum_p += p;
				_backward_sampling_table[table_index] = p;
				table_index++;
				// p_k.push_back(p);
			}
			if(t - k == 0){
				id word_j_id = ID_BOS;
				id word_k_id = get_substring_word_id_at_t_k(sentence, t, k);
				id word_t_id = ID_EOS;
				if(t < sentence->size()){
					assert(t + next_word_length <= sentence->size());
					assert(next_word_length > 0);
					word_t_id = get_substring_word_id_at_t_k(sentence, t + next_word_length, next_word_length);
					// id word_id = sentence->get_substr_word_id(t - 1, t + next_word_length - 2);
					// assert(word_t_id == word_id);
				}
				_word_ids[0] = word_j_id;
				_word_ids[1] = word_k_id;
				_word_ids[2] = word_t_id;
				double pw_h = 0;
				if(t == sentence->size()){	// <eos>に接続する確率からサンプリング
					pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t, t);
				}else{
					pw_h = _pw_h[t + next_word_length][next_word_length][k][0];
					#ifdef __DEBUG__
						double pw_h2 = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t, t + next_word_length - 1);
						if(pw_h != pw_h2){
							std::cout << "t = " << t << ", k = " << k << ", j = " << 0 << std::endl;
							std::cout << "next_word_length = " << next_word_length << std::endl;
							std::cout << "size = " << sentence->size() << std::endl;
						}
						assert(pw_h == pw_h2);
					#endif
				}
				assert(_alpha[t][k][0] > 0);
				double p = pw_h * _alpha[t][k][0];
				assert(p > 0);
				sum_p += p;
				_backward_sampling_table[table_index] = p;
				table_index++;
			}
		}
		assert(table_index > 0);
		assert(table_index <= _max_word_length * _max_word_length);
		double normalizer = 1.0 / sum_p;
		double r = sampler::uniform(0, 1);
		int i = 0;
		double stack = 0;
		for(int k = 1;k <= limit_k;k++){
			for(int j = 1;j <= std::min(t - k, _max_word_length);j++){
				assert(i < table_index);
				assert(_backward_sampling_table[i] > 0);
				stack += _backward_sampling_table[i] * normalizer;
				if(r < stack){
					sampled_k = k;
					sampled_j = j;
					return;
				}
				i++;
			}
			if(t - k == 0){
				assert(i < table_index);
				assert(_backward_sampling_table[i] > 0);
				stack += _backward_sampling_table[i] * normalizer;
				if(r < stack){
					sampled_k = k;
					sampled_j = 0;
					return;
				}
				i++;
			}
		}
	}
	// Blocked Gibbs Samplingによる分割のサンプリング
	// 分割結果が確率的に決まる
	// use_scaling=trueでアンダーフローを防ぐ
	void Lattice::blocked_gibbs(Sentence* sentence, std::vector<int> &segments, bool use_scaling){
		assert(sentence->size() <= _max_sentence_length);
		int size = sentence->size() + 1;

		#ifdef __DEBUG__
			for(int t = 0;t < size;t++){
				_scaling[t] = 0;
				for(int k = 0;k < _max_word_length + 1;k++){
					for(int j = 0;j < _max_word_length + 1;j++){
						_alpha[t][k][j] = -1;
					}
				}
			}
			for(int k = 0;k < _max_word_length;k++){
				for(int j = 0;j < _max_word_length;j++){
					_backward_sampling_table[k * _max_word_length + j] = -1;
				}
			}
		#endif

		_alpha[0][0][0] = 1;
		_scaling[0] = 0;
		for(int i = 0;i < size;i++){
			for(int j = 0;j < _max_word_length + 1;j++){
				_substring_word_id_cache[i][j] = 0;
			}
		}
		forward_filtering(sentence, use_scaling);
		backward_sampling(sentence, segments);
	}
	// ビタビアルゴリズム用
	void Lattice::viterbi_argmax_alpha_t_k_j(Sentence* sentence, int t, int k, int j){
		id word_k_id = get_substring_word_id_at_t_k(sentence, t, k);
		wchar_t const* characters = sentence->_characters;
		int character_ids_length = sentence->size();
		assert(t <= character_ids_length + 1);
		assert(k <= _max_word_length);
		assert(j <= _max_word_length);
		assert(t - k >= 0);
		// <bos>から生成されている場合
		if(j == 0){
			_word_ids[0] = ID_BOS;
			_word_ids[1] = ID_BOS;
			_word_ids[2] = word_k_id;
			double pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t - k, t - 1);
			assert(pw_h > 0);
			_alpha[t][k][0] = log(pw_h);
			_viterbi_backward[t][k][0] = 0;
			return;
		}
		// i=0に相当
		if(t - k - j == 0){
			id word_j_id = get_substring_word_id_at_t_k(sentence, t - k, j);
			_word_ids[0] = ID_BOS;
			_word_ids[1] = word_j_id;
			_word_ids[2] = word_k_id;
			double pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t - k, t - 1);
			assert(pw_h > 0);
			assert(_alpha[t - k][j][0] != 0);
			_alpha[t][k][j] = log(pw_h) + _alpha[t - k][j][0];
			assert(_alpha[t][k][j] <= 0);
			_viterbi_backward[t][k][j] = 0;
			return;
		}
		// それ以外の場合は周辺化
		double max_log_p = 0;
		int argmax = 0;
		for(int i = 1;i <= std::min(t - k - j, _max_word_length);i++){
			id word_i_id = get_substring_word_id_at_t_k(sentence, t - k - j, i);
			id word_j_id = get_substring_word_id_at_t_k(sentence, t - k, j);
			_word_ids[0] = word_i_id;
			_word_ids[1] = word_j_id;
			_word_ids[2] = word_k_id;
			double pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t - k, t - 1);
			assert(pw_h > 0);
			assert(i <= _max_word_length);
			assert(_alpha[t - k][j][i] <= 0);
			double value = log(pw_h) + _alpha[t - k][j][i];
			assert(value <= 0);
			if(argmax == 0 || value > max_log_p){
				argmax = i;
				max_log_p = value;
			}
		}
		assert(argmax > 0);
		_alpha[t][k][j] = max_log_p;
		_viterbi_backward[t][k][j] = argmax;
	}
	void Lattice::viterbi_forward(Sentence* sentence){
		for(int t = 1;t <= sentence->size();t++){
			for(int k = 1;k <= std::min(t, _max_word_length);k++){
				if(t - k == 0){
					viterbi_argmax_alpha_t_k_j(sentence, t, k, 0);
				}
				for(int j = 1;j <= std::min(t - k, _max_word_length);j++){
					viterbi_argmax_alpha_t_k_j(sentence, t, k, j);
				}
			}
		}
	}
	// <eos>に繋がる確率でargmax
	void Lattice::viterbi_argmax_backward_k_and_j_to_eos(Sentence* sentence, int t, int next_word_length, int &argmax_k, int &argmax_j){
		assert(t == sentence->size());
		wchar_t const* characters = sentence->_characters;
		int character_ids_length = sentence->size();
		double max_log_p = 0;
		argmax_k = 0;
		argmax_j = 0;
		int limit_k = std::min(t, _max_word_length);
		for(int k = 1;k <= limit_k;k++){
			for(int j = 1;j <= std::min(t - k, _max_word_length);j++){
				id word_j_id = get_substring_word_id_at_t_k(sentence, t - k, j);
				id word_k_id = get_substring_word_id_at_t_k(sentence, t, k);
				_word_ids[0] = word_j_id;
				_word_ids[1] = word_k_id;
				_word_ids[2] = ID_EOS;
				double pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t, t);
				assert(_alpha[t][k][j] <= 0);
				double value = log(pw_h) + _alpha[t][k][j];
				assert(value <= 0);
				if(argmax_k == 0 || value > max_log_p){
					max_log_p = value;
					argmax_k = k;
					argmax_j = j;
				}
			}
			if(t - k == 0){
				id word_j_id = ID_BOS;
				id word_k_id = get_substring_word_id_at_t_k(sentence, t, k);
				id word_t_id = ID_EOS;
				_word_ids[0] = word_j_id;
				_word_ids[1] = word_k_id;
				_word_ids[2] = word_t_id;
				double pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t, t);
				assert(_alpha[t][k][0] <= 0);
				double value = log(pw_h) + _alpha[t][k][0];
				assert(value <= 0);
				if(argmax_k == 0 || value > max_log_p){
					max_log_p = value;
					argmax_k = k;
					argmax_j = 0;
				}
			}
		}
	}
	void Lattice::viterbi_backward(Sentence* sentence, std::vector<int> &segments){
		segments.clear();
		int k = 0;
		int j = 0;
		int sum = 0;
		int t = sentence->size();
		viterbi_argmax_backward_k_and_j_to_eos(sentence, t, 1, k, j);
		assert(k <= _max_word_length);
		segments.push_back(k);
		sum += k;
		if(j == 0 && k == t){	// 文章すべてが1単語になる場合
			assert(sum == sentence->size());
			return;
		}
		assert(k > 0 && j > 0);
		assert(j <= _max_word_length);
		segments.push_back(j);
		int i = _viterbi_backward[t][k][j];
		assert(i >= 0);
		assert(i <= _max_word_length);
		t -= k;
		sum += j;
		sum += i;
		k = j;
		j = i;
		if(i == 0){
			assert(sum == sentence->size());
			return;
		}
		segments.push_back(i);
		while(t > 0){
			i = _viterbi_backward[t][k][j];
			assert(i >= 0);
			assert(i <= _max_word_length);
			if(i != 0){
				segments.push_back(i);
			}
			// std::cout << "<- " << k << std::endl;
			t -= k;
			k = j;
			j = i;
			sum += i;
		}
		assert(t == 0);
		assert(sum == sentence->size());
		assert(segments.size() > 0);
		reverse(segments.begin(), segments.end());
	}
	// ビタビアルゴリズムによる分割
	// 決定的に分割が決まる
	void Lattice::viterbi_decode(Sentence* sentence, std::vector<int> &segments){
		assert(sentence->size() <= _max_sentence_length);
		int size = sentence->size() + 1;

		#ifdef __DEBUG__
			for(int t = 0;t < size;t++){
				for(int k = 0;k < _max_word_length + 1;k++){
					for(int j = 0;j < _max_word_length + 1;j++){
						_alpha[t][k][j] = -1;
					}
				}
				for(int k = 0;k < _max_word_length;k++){
					for(int j = 0;j < _max_word_length;j++){
						_viterbi_backward[t][k][j] = -1;
					}
				}
			}
		#endif

		_alpha[0][0][0] = 0;
		_scaling[0] = 0;
		for(int t = 0;t < size;t++){
			for(int k = 0;k < _max_word_length + 1;k++){
				_substring_word_id_cache[t][k] = 0;
			}
		}
		viterbi_forward(sentence);
		viterbi_backward(sentence, segments);
	}
	// 文の可能な分割全てを考慮した文の確率（<eos>への接続を含む）
	// use_scaling=trueならアンダーフローを防ぐ
	double Lattice::compute_marginal_p_x(Sentence* sentence, bool use_scaling){
		assert(sentence->size() <= _max_sentence_length);
		int size = sentence->size() + 1;
		#ifdef __DEBUG__
			for(int t = 0;t < size;t++){
				for(int k = 0;k < _max_word_length + 1;k++){
					for(int j = 0;j < _max_word_length + 1;j++){
						_alpha[t][k][j] = -1;
					}
				}
			}
		#endif 
		_alpha[0][0][0] = 1;
		_scaling[0] = 0;
		for(int i = 0;i < size;i++){
			for(int j = 0;j < _max_word_length + 1;j++){
				_substring_word_id_cache[i][j] = 0;
			}
		}
		_enumerate_forward_variables(sentence, _alpha, _pw_h, _scaling, use_scaling);
		double sum_probability = 0;
		int t = sentence->size();
		wchar_t const* characters = sentence->_characters;
		int const* character_ids = sentence->_character_ids;
		int character_ids_length = sentence->size();
		for(int k = 1;k <= std::min(t, _max_word_length);k++){
			for(int j = 1;j <= std::min(t - k, _max_word_length);j++){
				id word_j_id = get_substring_word_id_at_t_k(sentence, t - k, j);
				id word_k_id = get_substring_word_id_at_t_k(sentence, t, k);
				_word_ids[0] = word_j_id;
				_word_ids[1] = word_k_id;
				_word_ids[2] = ID_EOS;
				double pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t, t);
				assert(_alpha[t][k][j] > 0);
				double potential = _crf->compute_trigram_potential(character_ids, characters, character_ids_length, t + 1, 1, k);
				double p = exp(_lambda_0 * log(pw_h) + potential);
				assert(p > 0);
				sum_probability += _alpha[t][k][j] * p;
			}
		}
		return sum_probability;
	}
	// 文の可能な分割全てを考慮した文の確率（<eos>への接続を含む）
	// use_scaling=trueならアンダーフローを防ぐ
	double Lattice::_compute_marginal_p_x_backward(Sentence* sentence, double*** beta, double**** pw_h_tkji, double* scaling, bool use_scaling){
		assert(sentence->size() <= _max_sentence_length);
		int size = sentence->size() + 1;
		#ifdef __DEBUG__
			for(int t = 0;t < size;t++){
				for(int k = 0;k < _max_word_length + 1;k++){
					for(int j = 0;j < _max_word_length + 1;j++){
						beta[t][k][j] = -1;
					}
				}
			}
		#endif 
		beta[0][0][0] = 1;
		for(int i = 0;i < size;i++){
			for(int j = 0;j < _max_word_length + 1;j++){
				_substring_word_id_cache[i][j] = 0;
			}
		}
		_enumerate_backward_variables(sentence, beta, pw_h_tkji, scaling, use_scaling);
		double sum_probability = 0;
		wchar_t const* characters = sentence->_characters;
		int const* character_ids = sentence->_character_ids;
		int character_ids_length = sentence->size();
		for(int k = 1;k <= _max_word_length;k++){
			int t = k;
			id word_k_id = get_substring_word_id_at_t_k(sentence, t, k);
			_word_ids[0] = ID_BOS;
			_word_ids[1] = ID_BOS;
			_word_ids[2] = word_k_id;
			double pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, 0, t - 1);
			assert(pw_h > 0);
			double potential = _crf->compute_trigram_potential(character_ids, characters, character_ids_length, t, k, 0);
			double p = exp(_lambda_0 * log(pw_h) + potential);
			assert(p > 0);
			sum_probability += _beta[t][k][0] * p;
		}
		return sum_probability;
	}
	// 文の部分文字列が単語になる確率
	// 確率値は全て比例のまま
	// alphaとbetaは計算時に正規化を行なってはいけない（重要）
	void Lattice::_enumerate_proportional_p_substring_given_sentence(Sentence* sentence, double*** alpha, double*** beta, double** pc_s){
		assert(sentence->size() <= _max_sentence_length);
		int size = sentence->size() + 1;
		#ifdef __DEBUG__
			for(int t = 0;t < size;t++){
				for(int k = 0;k < _max_word_length + 1;k++){
						pc_s[t][k] = -1;
				}
			}
		#endif 
		for(int t = 1;t <= sentence->size();t++){
			for(int k = 1;k <= std::min(t, _max_word_length);k++){
				// jを網羅する
				double sum_probability = 0;
				for(int j = 1;j <= std::min(t - k, _max_word_length);j++){
					assert(alpha[t][k][j] > 0);
					assert(beta[t][k][j] > 0);
					sum_probability += alpha[t][k][j] * beta[t][k][j];
				}
				pc_s[t][k] = sum_probability;
			}
		}
	}
	// 文の部分文字列が単語になる確率
	// 確率値は全て比例のまま
	// アンダーフローを抑えるためにlogで計算
	// log_zは各時刻の正規化定数
	// alphaとbetaは計算時に正規化を行っている必要がある（重要）
	void Lattice::_enumerate_proportional_log_p_substring_given_sentence(Sentence* sentence, double*** alpha, double*** beta, double* log_z_alpha, double* log_z_beta, double** pc_s){
		assert(sentence->size() <= _max_sentence_length);
		int size = sentence->size() + 1;
		#ifdef __DEBUG__
			for(int t = 0;t < size;t++){
				for(int k = 0;k < _max_word_length + 1;k++){
						pc_s[t][k] = -1;
				}
			}
		#endif 
		wchar_t const* characters = sentence->_characters;
		int const* character_ids = sentence->_character_ids;
		int character_ids_length = sentence->size();
		for(int t = 1;t <= sentence->size();t++){
			for(int k = 1;k <= std::min(t, _max_word_length);k++){
				for(int j = 1;j <= std::min(t - k, _max_word_length);j++){
					assert(alpha[t][k][j] > 0);
					assert(beta[t][k][j] > 0);
					assert(log_z_alpha[t] < 0);
					assert(log_z_beta[t] < 0);
				}
			}
		}
	}
	void Lattice::_enumerate_forward_variables(Sentence* sentence, double*** alpha, double**** pw_h_tkji, double* scaling, bool use_scaling){
		assert(sentence->size() <= _max_sentence_length);
		int size = sentence->size() + 1;
		for(int i = 0;i < size;i++){
			for(int j = 0;j < _max_word_length + 1;j++){
				_substring_word_id_cache[i][j] = 0;
			}
		}
		#ifdef __DEBUG__
			// 変な値を入れる
			for(int t = 0;t < size;t++){
				scaling[t] = -1;
				for(int k = 0;k < _max_word_length + 1;k++){
					for(int j = 0;j < _max_word_length + 1;j++){
						alpha[t][k][j] = -1;
					}
				}
			}
		#endif 
		// <eos>未満の前向き確率を計算
		alpha[0][0][0] = 1;
		for(int t = 1;t <= sentence->size();t++){
			double prod_scaling = 1;
			for(int k = 1;k <= std::min(t, _max_word_length);k++){
				if(use_scaling == true && k > 1){
					prod_scaling *= scaling[t - k + 1];
				}
				for(int j = (t - k == 0) ? 0 : 1;j <= std::min(t - k, _max_word_length);j++){
					_sum_alpha_t_k_j(sentence, t, k, j, alpha, pw_h_tkji, prod_scaling);
				}
			}
			// スケーリング
			if(use_scaling == true){
				double sum_alpha = 0;
				for(int k = 1;k <= std::min(t, _max_word_length);k++){
					for(int j = (t - k == 0) ? 0 : 1;j <= std::min(t - k, _max_word_length);j++){
						assert(alpha[t][k][j] > 0);
						sum_alpha += alpha[t][k][j];
					}
				}
				assert(sum_alpha > 0);
				scaling[t] = 1.0 / sum_alpha;
				for(int k = 1;k <= std::min(t, _max_word_length);k++){
					for(int j = (t - k == 0) ? 0 : 1;j <= std::min(t - k, _max_word_length);j++){
						alpha[t][k][j] *= scaling[t];
					}
				}
			}
		}
		// <eos>への接続を考える
		double alpha_eos = 0;
		int t = sentence->size() + 1; // <eos>を指す
		int k = 1;	// ここでは<eos>の長さを1と考える
		wchar_t const* characters = sentence->_characters;
		int character_ids_length = sentence->size();
		for(int j = 1;j <= std::min(t - k, _max_word_length);j++){
			double sum_prob = 0;
			for(int i = (t - k - j == 0) ? 0 : 1;i <= std::min(t - k - j, _max_word_length);i++){
				_word_ids[0] = get_substring_word_id_at_t_k(sentence, t - k - j, i);
				_word_ids[1] = get_substring_word_id_at_t_k(sentence, t - k, j);
				_word_ids[2] = ID_EOS;
				double pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t, t);
				assert(pw_h > 0);
				sum_prob += pw_h * alpha[t - k][j][i];
			}
			alpha[t][k][j] = sum_prob;
			alpha_eos += sum_prob;
		}
		if(use_scaling){
			scaling[t] = 1.0 / alpha_eos;
		}
	}
	void Lattice::_enumerate_backward_variables(Sentence* sentence, double*** beta, double**** pw_h_tkji, double* scaling, bool use_scaling){
		assert(sentence->size() <= _max_sentence_length);
		int size = sentence->size() + 1;
		#ifdef __DEBUG__
			for(int t = 0;t < size;t++){
				for(int k = 0;k < _max_word_length + 1;k++){
					for(int j = 0;j < _max_word_length + 1;j++){
						beta[t][k][j] = -1;
					}
				}
			}
		#endif 
		scaling[0] = 0;
		for(int t = 0;t < size;t++){
			for(int k = 0;k < _max_word_length + 1;k++){
				_substring_word_id_cache[t][k] = 0;
			}
		}
		for(int t = sentence->size() - 1;t >= 1;t--){
			for(int k = 1;k <= std::min(t, _max_word_length);k++){
				for(int j = (t - k == 0) ? 0 : 1;j <= std::min(t - k, _max_word_length);j++){
					beta[t][k][j] = 0;
					_sum_beta_t_k_j(sentence, t, k, j, beta, pw_h_tkji);
				}
			}
			// 正規化
			if(use_scaling){
			}
		}
	}
	// 後ろ向き確率を計算
	// 正規化定数をここでは掛けないことに注意
	// pw_h_tkjiは前向き確率計算時にキャッシュされている（-1が入っている場合再計算する）
	void Lattice::_sum_beta_t_k_j(Sentence* sentence, int t, int k, int j, double*** beta, double**** pw_h_tkji){
		id word_k_id = get_substring_word_id_at_t_k(sentence, t, k);
		wchar_t const* characters = sentence->_characters;
		int const* character_ids = sentence->_character_ids;
		int character_ids_length = sentence->size();
		assert(1 <= t && t <= character_ids_length);
		assert(1 <= k && k <= _max_word_length);
		assert(0 <= j && j <= _max_word_length);
		assert(t - k >= 0);
		// <eos>へ接続する場合
		if(t == sentence->size()){
			_word_ids[0] = ID_BOS;
			_word_ids[1] = word_k_id;
			_word_ids[2] = ID_EOS;
			if(j > 0){
				id word_j_id = get_substring_word_id_at_t_k(sentence, t - k, j);
				_word_ids[0] = word_j_id;
			}
			double pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t, t);
			assert(pw_h > 0);
			double potential = _crf->compute_trigram_potential(character_ids, characters, character_ids_length, t + 1, 1, k);
			double p = exp(_lambda_0 * log(pw_h) + potential);
			assert(p > 0);
			beta[t][k][j] = p;
			// assert(_pw_h[t][k][j][0] == _pw_h);
			// _pw_h[t][k][0][0] = p;
			return;
		}
		// それ以外の場合は周辺化
		double sum = 0;
		for(int i = 1;i <= std::min(sentence->size() - t, _max_word_length);i++){
			id word_i_id = get_substring_word_id_at_t_k(sentence, t + i, i);
			id word_j_id = get_substring_word_id_at_t_k(sentence, t - k, j);
			_word_ids[0] = word_j_id;
			_word_ids[1] = word_k_id;
			_word_ids[2] = word_i_id;
			double pw_h = (pw_h_tkji[t + i][i][k][j] > 0) ? pw_h_tkji[t + i][i][k][j] : _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t, t + i - 1);
			assert(pw_h > 0);
			double potential = _crf->compute_trigram_potential(character_ids, characters, character_ids_length, t, k, j);
			double p = exp(_lambda_0 * log(pw_h) + potential);
			assert(p > 0);
			assert(beta[t + i][i][k] > 0);
			double value = p * beta[t + i][i][k];
			assert(p > 0);

			#ifdef __DEBUG__
				if(pw_h_tkji[t + i][i][k][j] > 0){
					double _pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t, t + i - 1);
					assert(_pw_h == pw_h_tkji[t + i][i][k][j]);
				}
				if(value == 0){
					std::cout << pw_h << std::endl;
					std::cout << beta[t + i][i][k] << std::endl;
					std::cout << t << ", " << k << ", " << j << ", " << i << std::endl;
				}
			#endif

			assert(value > 0);
			sum += value;
			// assert(_pw_h[t + i][i][k][j] == pw_h);
		}
		assert(sum > 0);
		beta[t][k][j] = sum;
		return;



		// if(j == 0){
		// 	_word_ids[0] = ID_BOS;
		// 	_word_ids[1] = ID_BOS;
		// 	_word_ids[2] = word_k_id;
		// 	double pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t - k, t - 1);
		// 	assert(pw_h > 0);
		// 	double potential = _crf->compute_trigram_potential(character_ids, characters, character_ids_length, t, k, j);
		// 	double p = exp(_lambda_0 * log(pw_h) + potential);
		// 	assert(p > 0);
		// 	_alpha[t][k][0] = p;
		// 	_pw_h[t][k][0][0] = p;
		// 	return;
		// }
		// // i=0に相当
		// if(t - k - j == 0){
		// 	id word_j_id = get_substring_word_id_at_t_k(sentence, t - k, j);
		// 	_word_ids[0] = ID_BOS;
		// 	_word_ids[1] = word_j_id;
		// 	_word_ids[2] = word_k_id;
		// 	double pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t - k, t - 1);
		// 	assert(pw_h > 0);
		// 	double potential = _crf->compute_trigram_potential(character_ids, characters, character_ids_length, t, k, j);
		// 	double p = exp(_lambda_0 * log(pw_h) + potential);
		// 	assert(p > 0);
		// 	assert(_alpha[t - k][j][0] > 0);
		// 	_alpha[t][k][j] = p * _alpha[t - k][j][0];
		// 	assert(_alpha[t][k][j] > 0);
		// 	_pw_h[t][k][j][0] = p;
		// 	return;
		// }
		// // それ以外の場合は周辺化
		// double sum = 0;
		// for(int i = 1;i <= std::min(t - k - j, _max_word_length);i++){
		// 	id word_i_id = get_substring_word_id_at_t_k(sentence, t - k - j, i);
		// 	id word_j_id = get_substring_word_id_at_t_k(sentence, t - k, j);
		// 	_word_ids[0] = word_i_id;
		// 	_word_ids[1] = word_j_id;
		// 	_word_ids[2] = word_k_id;
		// 	double pw_h = _npylm->compute_p_w_given_h(characters, character_ids_length, _word_ids, 3, 2, t - k, t - 1);
		// 	assert(i <= _max_word_length);
		// 	double potential = _crf->compute_trigram_potential(character_ids, characters, character_ids_length, t, k, j);
		// 	double p = exp(_lambda_0 * log(pw_h) + potential);
		// 	assert(p > 0);
		// 	assert(_alpha[t - k][j][i] > 0);
		// 	double value = p * _alpha[t - k][j][i];
		// 	assert(p > 0);

		// 	#ifdef __DEBUG__
		// 		if(value == 0){
		// 			std::cout << pw_h << std::endl;
		// 			std::cout << _alpha[t - k][j][i] << std::endl;
		// 			std::cout << t << ", " << k << ", " << j << ", " << i << std::endl;
		// 		}
		// 	#endif

		// 	assert(value > 0);
		// 	sum += value;
		// 	_pw_h[t][k][j][i] = p;
		// }
		// assert(sum > 0);
		// _alpha[t][k][j] = sum;
	}
	void Lattice::_clear_pw_h_tkji(double**** pw_h_tkji){
		int size = _max_sentence_length + 1;
		for(int t = 0;t < size;t++){
			assert(_pw_h[t] != NULL);
			for(int k = 0;k < _max_word_length + 1;k++){
				for(int j = 0;j < _max_word_length + 1;j++){
					for(int i = 0;i < _max_word_length + 1;i++){
						_pw_h[t][k][j][i] = -1;
					}
				}
			}
		}
	}
} // namespace npylm