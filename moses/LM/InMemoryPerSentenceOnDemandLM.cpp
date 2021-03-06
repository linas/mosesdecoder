#include <boost/foreach.hpp>
#include "InMemoryPerSentenceOnDemandLM.h"
#include "moses/FactorCollection.h"
#include "moses/Util.h"
#include "moses/StaticData.h"
#include "moses/TranslationTask.h"
#include "moses/ContextScope.h"
#include "moses/LM/Ken.h"
#include "lm/model.hh"
#include "util/mmap.hh"

#include <cstdio>
#include <iostream>
#include <fstream>

using namespace std;

namespace Moses
{
InMemoryPerSentenceOnDemandLM::InMemoryPerSentenceOnDemandLM(const std::string &line) : LanguageModel(line), initialized(false)
{
  ReadParameters();
}

InMemoryPerSentenceOnDemandLM::~InMemoryPerSentenceOnDemandLM()
{
}

void InMemoryPerSentenceOnDemandLM::InitializeForInput(ttasksptr const& ttask)
{

  // The context scope object for this translation task
  //     contains a map of translation task-specific data
  boost::shared_ptr<Moses::ContextScope> contextScope = ttask->GetScope();

  // The key to the map is this object
  void const* key = static_cast<void const*>(this);

  // The value stored in the map is a string representing a phrase table
  boost::shared_ptr<string> value = contextScope->get<string>(key);

  // Create a stream to read the phrase table data
  stringstream strme(*(value.get()));

  char * nullpointer = (char *) 0;
  const char * filename = std::tmpnam(nullpointer);
  ofstream tmp;
  tmp.open(filename);

  // Read the phrase table data, one line at a time
  string line;
  while (getline(strme, line)) {

    tmp << line << "\n";

  }

  tmp.close();

  LanguageModelKen<lm::ngram::ProbingModel> & lm = GetPerThreadLM();
  lm.LoadModel("/home/lanes/mosesdecoder/tiny.with_per_sentence/europarl.en.srilm", util::POPULATE_OR_READ);

  initialized = true;

  VERBOSE(1, filename);
  if (initialized) {
    VERBOSE(1, "\tLM initialized\n");
  }

  //  std::remove(filename);

}

LanguageModelKen<lm::ngram::ProbingModel>& InMemoryPerSentenceOnDemandLM::GetPerThreadLM() const
{

  LanguageModelKen<lm::ngram::ProbingModel> *lm;
  lm = m_perThreadLM.get();
  if (lm == NULL) {
    lm = new LanguageModelKen<lm::ngram::ProbingModel>();
    m_perThreadLM.reset(lm);
  }
  assert(lm);
  return *lm;

}



}



