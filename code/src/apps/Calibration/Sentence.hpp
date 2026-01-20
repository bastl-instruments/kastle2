/*
MIT License

Copyright (c) 2024 Vaclav Mach (Bastl Instruments)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <vector>
#include "common/dsp/sampling/SamplePlayer.hpp"

namespace kastle2
{

/**
 * @class Sentence
 * @brief Sentence made of audio samples (=Words).
 * @author Vaclav Mach, Bastl Instruments
 * @date 2024-09-02
 * This class manages a sequence of audio samples (words) that can be played back in order.
 * It keeps track of the current index and allows retrieving the next word until all words are
 * played. It also provides functionality to reset the index to replay the sentence.
 */
class Sentence
{

public:
    /**
     * @brief A single audio sample represented as a Word.
     */
    using Word = SamplePlayer16bit::Sample;

    /**
     * @brief A vector of Word objects representing the audio samples in the sentence.
     * This is used to store the sequence of words that make up the sentence.
     */
    using Words = std::vector<Word>;

    /**
     * @brief Default constructor for Sentence.
     * Initializes an empty sentence with no words and index set to 0.
     */
    Sentence() : words_{}, index(0) {}

    /**
     * @brief Constructs a Sentence with a list of words.
     * @param words A vector of Word objects representing the audio samples in the sentence.
     */
    Sentence(Words words)
    {
        words_ = words;
        index = 0;
    }

    /**
     * @brief Retrieves the next word in the sentence.
     * @return The next Word object or an empty Word if no more words are available.
     */
    Word GetNext()
    {
        if (index >= words_.size())
        {
            return Word{nullptr, 0};
        }
        Word entry = words_[index];
        index++;
        return entry;
    }

    /**
     * @brief Checks if the sentence has finished playing.
     * @return True if all words have been played, false otherwise.
     */
    bool IsFinished()
    {
        return index >= words_.size();
    }

    /**
     * @brief Resets the index to the beginning of the sentence.
     * This allows replaying the sentence from the start.
     */
    void Reset()
    {
        index = 0;
    }

private:
    Words words_;
    size_t index;
};
}
