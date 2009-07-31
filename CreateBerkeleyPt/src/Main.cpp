#include <iostream>
#include <string>
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Vocab.h"
#include "DbWrapper.h"
#include "Global.h"
#include "../../moses/src/InputFileStream.h"
#include "../../moses/src/Util.h"
#include "../../moses/src/UserMessage.h"

using namespace std;

int main (int argc, char * const argv[]) {
    // insert code here...
  std::cerr << "Starting\n";
	
	string filePath = "pt.txt";
	
	Moses::InputFileStream inStream(filePath);

	DbWrapper dbWrapper;
	dbWrapper.Open(".");

	Global::Instance().Save(dbWrapper.GetSDbMisc());
	Global::Instance().Load(dbWrapper.GetSDbMisc());
	
	size_t numElement = NOT_FOUND; // 3=old format, 5=async format which include word alignment info

	string line;
	size_t lineNum = 0;
	size_t numScores = NOT_FOUND;
	
	while(getline(inStream, line))
	{
		line = Moses::Trim(line);
		if (line.size() == 0)
			continue;
		vector<string> tokens = Moses::TokenizeMultiCharSeparator( line , "|||" );
		
		// words
		const string &headWordsStr		= tokens[0]
								,&sourcePhraseStr	= tokens[1]
								,&targetPhraseStr	= tokens[2]
								,&alignStr				= tokens[3]
								,&scoresStr				= tokens[4];
						
		Phrase sourcePhrase;
		TargetPhrase targetPhrase;
		sourcePhrase.CreateFromString(sourcePhraseStr);
		targetPhrase.CreateFromString(targetPhraseStr);
		targetPhrase.CreateAlignFromString(alignStr);
		targetPhrase.CreateScoresFromString(scoresStr);
		targetPhrase.CreateHeadwordsFromString(headWordsStr);

		dbWrapper.SaveSource(sourcePhrase, targetPhrase);
		dbWrapper.SaveTarget(targetPhrase);
	}
	
	dbWrapper.Save(Vocab::Instance());
	dbWrapper.GetAllVocab();
	
	std::cerr << "Finished\n";
  return 0;
}


