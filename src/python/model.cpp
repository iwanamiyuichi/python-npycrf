#include <iostream>
#include "../npycrf/common.h"
#include "model.h"

using namespace npycrf::npylm;
using namespace npycrf::crf;

namespace npycrf {
	namespace python {
		Model::Model(model::NPYLM* py_npylm, model::CRF* py_crf, double lambda_0, int max_word_length, int max_sentence_length){
			_set_locale();
			_npylm = py_npylm->_npylm;
			_crf = py_crf->_crf;
			_lattice = new Lattice(_npylm, _crf, lambda_0, max_word_length, max_sentence_length);
			_lambda_0 = lambda_0;
		}
		Model::~Model(){

		}
		// 日本語周り
		void Model::_set_locale(){
			setlocale(LC_CTYPE, "ja_JP.UTF-8");
			std::ios_base::sync_with_stdio(false);
			std::locale default_loc("ja_JP.UTF-8");
			std::locale::global(default_loc);
			std::locale ctype_default(std::locale::classic(), default_loc, std::locale::ctype); //※
			std::wcout.imbue(ctype_default);
			std::wcin.imbue(ctype_default);
		}
		int Model::get_max_word_length(){
			return _npylm->_max_word_length;
		}
		void Model::set_initial_lambda_a(double lambda){
			_npylm->_lambda_a = lambda;
			_npylm->sample_lambda_with_initial_params();
		}
		void Model::set_initial_lambda_b(double lambda){
			_npylm->_lambda_b = lambda;
			_npylm->sample_lambda_with_initial_params();
		}
		void Model::set_vpylm_beta_stop(double stop){
			_npylm->_vpylm->_beta_stop = stop;
		}
		void Model::set_vpylm_beta_pass(double pass){
			_npylm->_vpylm->_beta_pass = pass;
		}
		// 分配関数の計算
		// normalize=trueならアンダーフローを防ぐ
		double Model::compute_z_x(Sentence* sentence, bool normalize){
			// キャッシュの再確保
			_lattice->reserve(_npylm->_max_word_length, sentence->size());
			_npylm->reserve(sentence->size());
			double zx = _lattice->compute_z_x(sentence, normalize);
			return zx;
		}
		// sentenceは分割済みの必要がある
		// 比例のままの確率を返す
		double Model::compute_log_p_proportional_y_given_x(Sentence* sentence){
			double log_crf = 0;
			double log_npylm = _npylm->compute_log_p_s(sentence);
			double log_py_x = log_crf + _lambda_0 * log_npylm;
			return log_py_x;
		}
		// normalize=trueならアンダーフローを防ぐ
		double Model::compute_forward_probability(std::wstring sentence_str, Dictionary* dictionary, bool normalize){
			// キャッシュの再確保
			_lattice->reserve(_npylm->_max_word_length, sentence_str.size());
			_npylm->reserve(sentence_str.size());
			// 構成文字を辞書に追加し、文字IDに変換
			int* character_ids = new int[sentence_str.size()];
			for(int i = 0;i < sentence_str.size();i++){
				wchar_t character = sentence_str[i];
				int character_id = dictionary->get_character_id(character);
				character_ids[i] = character_id;
			}
			Sentence* sentence = new Sentence(sentence_str, character_ids);
			double probability = _lattice->compute_forward_probability(sentence, normalize);
			delete[] character_ids;
			delete sentence;
			return probability;
		}
		// normalize=trueならアンダーフローを防ぐ
		double Model::compute_backward_probability(std::wstring sentence_str, Dictionary* dictionary, bool normalize){
			// キャッシュの再確保
			_lattice->reserve(_npylm->_max_word_length, sentence_str.size());
			_npylm->reserve(sentence_str.size());
			// 構成文字を辞書に追加し、文字IDに変換
			int* character_ids = new int[sentence_str.size()];
			for(int i = 0;i < sentence_str.size();i++){
				wchar_t character = sentence_str[i];
				int character_id = dictionary->get_character_id(character);
				character_ids[i] = character_id;
			}
			Sentence* sentence = new Sentence(sentence_str, character_ids);
			double probability = _lattice->compute_backward_probability(sentence, normalize);
			delete[] character_ids;
			delete sentence;
			return probability;
		}
		void Model::parse(Sentence* sentence){
			// キャッシュの再確保
			_lattice->reserve(_npylm->_max_word_length, sentence->size());
			_npylm->reserve(sentence->size());
			std::vector<int> segments;		// 分割の一時保存用
			_lattice->viterbi_decode(sentence, segments);
			sentence->split(segments);
		}
		boost::python::list Model::python_parse(std::wstring sentence_str, Dictionary* dictionary){
			// キャッシュの再確保
			_lattice->reserve(_npylm->_max_word_length, sentence_str.size());
			_npylm->reserve(sentence_str.size());
			std::vector<int> segments;		// 分割の一時保存用
			// 構成文字を辞書に追加し、文字IDに変換
			int* character_ids = new int[sentence_str.size()];
			for(int i = 0;i < sentence_str.size();i++){
				wchar_t character = sentence_str[i];
				int character_id = dictionary->get_character_id(character);
				character_ids[i] = character_id;
			}
			Sentence* sentence = new Sentence(sentence_str, character_ids);
			_lattice->viterbi_decode(sentence, segments);
			sentence->split(segments);
			boost::python::list words;
			for(int n = 0;n < sentence->get_num_segments_without_special_tokens();n++){
				std::wstring word = sentence->get_word_str_at(n + 2);
				words.append(word);
			}
			delete[] character_ids;
			delete sentence;
			return words;
		}
	}
}