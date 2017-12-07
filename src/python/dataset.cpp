#include <boost/python.hpp>
#include <iostream>
#include <fstream>
#include "dataset.h"
#include "../npycrf/sampler.h"
#include "../npycrf/hash.h"

namespace npycrf {
	namespace python {
		Dataset::Dataset(Corpus* corpus, Dictionary* dict, double train_split, int seed){
			_corpus = corpus;
			_max_sentence_length = 0;
			_avg_sentence_length = 0;
			int sum_sentence_length = 0;

			// まず教師なし学習用のデータをtrain/devに振り分ける
			_num_unsupervised_data = corpus->get_num_unsupervised_data();
			std::vector<int> rand_indices_u;
			for(int i = 0;i < _num_unsupervised_data;i++){
				rand_indices_u.push_back(i);
			}
			sampler::set_seed(seed);
			shuffle(rand_indices_u.begin(), rand_indices_u.end(), sampler::mt);	// データをシャッフル
			train_split = std::min(1.0, std::max(0.0, train_split));
			int num_train_data = _num_unsupervised_data * train_split;
			for(int i = 0;i < rand_indices_u.size();i++){
				std::wstring &sentence_str = corpus->_sentence_str_list[rand_indices_u[i]];
				if(i < num_train_data){
					_add_words_to_dataset(sentence_str, _sentence_sequences_train, dict);
				}else{
					_add_words_to_dataset(sentence_str, _sentence_sequences_dev, dict);
				}
				// 統計
				if(_max_sentence_length == 0 || sentence_str.size() > _max_sentence_length){
					_max_sentence_length = sentence_str.size();
				}
				sum_sentence_length += sentence_str.size();
			}
			
			// 教師あり学習用のデータをtrain/devに振り分ける
			_num_supervised_data = corpus->get_num_supervised_data();
			std::vector<int> rand_indices_l;
			shuffle(rand_indices_l.begin(), rand_indices_l.end(), sampler::mt);	// データをシャッフル
			train_split = std::min(1.0, std::max(0.0, train_split));
			num_train_data = _num_supervised_data * train_split;
			for(int i = 0;i < _num_supervised_data;i++){
				rand_indices_l.push_back(i);
			}
			for(int i = 0;i < rand_indices_l.size();i++){
				// 分割から元の文を復元
				std::vector<std::wstring> &words = corpus->_word_sequence_list[i];
				std::vector<int> segmentation;
				std::wstring sentence_str;
				for(auto word_str: words){
					sentence_str += word_str;
					segmentation.push_back(word_str.size());
				}
				// 構成文字を辞書に追加し、文字IDに変換
				int* character_ids = new int[sentence_str.size()];
				for(int i = 0;i < sentence_str.size();i++){
					wchar_t character = sentence_str[i];
					int character_id = dict->add_character(character);
					character_ids[i] = character_id;
				}
				// データセットに追加
				Sentence* sentence = new Sentence(sentence_str, character_ids, true);
				sentence->split(segmentation);		// 分割
				if(i < num_train_data){
					_sentence_sequences_train.push_back(sentence);
				}else{
					_sentence_sequences_dev.push_back(sentence);
				}
				// 統計
				if(_max_sentence_length == 0 || sentence_str.size() > _max_sentence_length){
					_max_sentence_length = sentence_str.size();
				}
				sum_sentence_length += sentence_str.size();
			}
			_avg_sentence_length = sum_sentence_length / (double)(_num_supervised_data + _num_unsupervised_data);
		}
		Dataset::~Dataset(){
			for(int n = 0;n < _sentence_sequences_train.size();n++){
				Sentence* sentence = _sentence_sequences_train[n];
				delete sentence;
			}
			for(int n = 0;n < _sentence_sequences_dev.size();n++){
				Sentence* sentence = _sentence_sequences_dev[n];
				delete sentence;
			}
		}
		int Dataset::get_num_training_data(){
			return _sentence_sequences_train.size();
		}
		int Dataset::get_num_validation_data(){
			return _sentence_sequences_dev.size();
		}
		void Dataset::_add_words_to_dataset(std::wstring &sentence_str, std::vector<Sentence*> &dataset, Dictionary* dict){
			assert(sentence_str.size() > 0);
			// 構成文字を辞書に追加し、文字IDに変換
			int* character_ids = new int[sentence_str.size()];
			for(int i = 0;i < sentence_str.size();i++){
				wchar_t character = sentence_str[i];
				int character_id = dict->add_character(character);
				character_ids[i] = character_id;
			}
			Sentence* sentence = new Sentence(sentence_str, character_ids);
			dataset.push_back(sentence);
		}
		int Dataset::get_max_sentence_length(){
			return _max_sentence_length;
		}
		int Dataset::get_average_sentence_length(){
			return _avg_sentence_length;
		}
		int Dataset::detect_hash_collision(int max_word_length){
			int step = 0;
			std::unordered_map<id, std::wstring> pool;
			for(Sentence* sentence: _sentence_sequences_train){
				if (PyErr_CheckSignals() != 0) {		// ctrl+cが押されたかチェック
					return 0;
				}
				_detect_collision_of_sentence(sentence, pool, max_word_length);
				step++;
			}
			for(Sentence* sentence: _sentence_sequences_dev){
				if (PyErr_CheckSignals() != 0) {		// ctrl+cが押されたかチェック
					return 0;
				}
				_detect_collision_of_sentence(sentence, pool, max_word_length);
				step++;
			}
			return pool.size();
		}
		void Dataset::_detect_collision_of_sentence(Sentence* sentence, std::unordered_map<id, std::wstring> &pool, int max_word_length){
			for(int t = 1;t <= sentence->size();t++){
				for(int k = 1;k <= std::min(t, max_word_length);k++){
					id word_id = sentence->get_substr_word_id(t - k, t - 1);
					std::wstring word = sentence->get_substr_word_str(t - k, t - 1);
					assert(word_id == hash_wstring(word));
					auto itr = pool.find(word_id);
					if(itr == pool.end()){
						pool[word_id] = word;
					}else{
						assert(itr->second == word);
					}
				}
			}
		}
	}
}