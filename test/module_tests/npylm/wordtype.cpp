#include <boost/functional/hash.hpp>
#include <iostream>
#include <fstream>
#include <cassert>
#include <unordered_set>
#include <string>
#include "../../../src/npycrf/ctype.h"
#include "../../../src/npycrf/wordtype.h"
using std::cout;
using std::flush;
using std::endl;

void test_wordtype(){
	std::wstring sentence_str = L"本論文では,100教師データや！？dictionaryを必要とせず,";
	int type;
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 0, 0);
	assert(type == WORDTYPE_KANJI);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 0, 1);
	assert(type == WORDTYPE_KANJI);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 0, 2);
	assert(type == WORDTYPE_KANJI);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 3, 3);
	assert(type == WORDTYPE_HIRAGANA);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 3, 4);
	assert(type == WORDTYPE_HIRAGANA);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 0, 3);
	assert(type == WORDTYPE_KANJI_HIRAGANA);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 0, 4);
	assert(type == WORDTYPE_KANJI_HIRAGANA);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 0, 5);
	assert(type == WORDTYPE_OTHER);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 0, 6);
	assert(type == WORDTYPE_OTHER);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 0, 7);
	assert(type == WORDTYPE_OTHER);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 6, 6);
	assert(type == WORDTYPE_NUMBER);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 6, 7);
	assert(type == WORDTYPE_NUMBER);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 6, 8);
	assert(type == WORDTYPE_NUMBER);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 6, 9);
	assert(type == WORDTYPE_OTHER);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 9, 11);
	assert(type == WORDTYPE_KANJI_KATAKANA);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 9, 12);
	assert(type == WORDTYPE_KANJI_KATAKANA);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 11, 11);
	assert(type == WORDTYPE_KATAKANA);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 11, 12);
	assert(type == WORDTYPE_KATAKANA);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 13, 14);
	assert(type == WORDTYPE_OTHER);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 17, 18);
	assert(type == WORDTYPE_ALPHABET);
	type = npylm::wordtype::detect_word_type_substr(sentence_str.data(), 26, 27);
	assert(type == WORDTYPE_OTHER);
}

int main(){
	test_wordtype();
	cout << "OK" << endl;
	return 0;
}