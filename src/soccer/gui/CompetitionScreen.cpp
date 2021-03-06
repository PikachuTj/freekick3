#include <algorithm>
#include <fstream>

#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "soccer/Match.h"
#include "soccer/Team.h"
#include "soccer/League.h"
#include "soccer/DataExchange.h"
#include "soccer/gui/Menu.h"
#include "soccer/gui/CompetitionScreen.h"

namespace Soccer {

CompetitionScreen::CompetitionScreen(boost::shared_ptr<ScreenManager> sm, const std::string& name,
		boost::shared_ptr<StatefulCompetition> l,
		bool onlyOneRound)
	: Screen(sm),
	mCompetitionName(name),
	mScreenName(name + " Screen"),
	mCompetition(l),
	mOneRound(onlyOneRound)
{
	addButton("Back",  Common::Rectangle(0.01f, 0.90f, 0.23f, 0.06f),
			true, SDLK_b);
	if(!mOneRound)
		addButton("Save",   Common::Rectangle(0.01f, 0.83f, 0.23f, 0.06f),
				true, SDLK_s, KMOD_CTRL);
	mSkipButton         = addButton("Skip",       Common::Rectangle(0.26f, 0.90f, 0.23f, 0.06f),
			true, SDLK_s);
	mResultButton       = addButton("Result",     Common::Rectangle(0.51f, 0.90f, 0.23f, 0.06f),
			true, SDLK_r);
	mMatchButton        = addButton("Match",      Common::Rectangle(0.76f, 0.90f, 0.23f, 0.06f),
			true, SDLK_m);
	if(!mOneRound)
		mNextRoundButton = addButton("Next Round", Common::Rectangle(0.26f, 0.90f, 0.73f, 0.06f),
				true, SDLK_n);

	updateRoundMatches();
}

void CompetitionScreen::addResultLabels(int a, int b, float xp, float yp,
		float fontsize, Screen& scr, std::vector<boost::shared_ptr<Button>>& labels,
		const char* suffix)
{
	if(!suffix) {
		labels.push_back(scr.addLabel(" - ", xp, yp, TextAlignment::Centered, fontsize, Common::Color::White));
		labels.push_back(scr.addLabel(std::to_string(a).c_str(), xp - 0.01f, yp,
					TextAlignment::Centered, fontsize, Common::Color::White));
		labels.push_back(scr.addLabel(std::to_string(b).c_str(), xp + 0.01f, yp,
					TextAlignment::Centered, fontsize, Common::Color::White));
	} else {
		std::stringstream ss;
		ss << "(" << a << "-" << b << suffix << ")";
		labels.push_back(scr.addLabel(ss.str().c_str(), xp, yp, TextAlignment::Centered, fontsize, Common::Color::White));
	}
}

float CompetitionScreen::addMatchLabels(const Match& m, float xp, float yp,
		float fontsize, Screen& scr, std::vector<boost::shared_ptr<Button>>& labels,
		bool rowsAvailable)
{
	float ret = 0.0f;
	static const Common::Color humancolor(192, 192, 192);
	static const Common::Color woncolor(255, 247, 125);
	bool played = m.getCupEntry().numMatchesPlayed() > 0;
	bool homefirst = (m.getCupEntry().numMatchesPlayed() & 1) == 1;
	const Common::Color& textColor1 = m.getTeam(0)->getController().HumanControlled ?
		(played && (m.getCupEntry().firstWon() == homefirst)) ? woncolor : humancolor :
		(played && (m.getCupEntry().firstWon() == homefirst)) ? woncolor : Common::Color::White;
	const Common::Color& textColor2 = m.getTeam(1)->getController().HumanControlled ?
		(played && (!m.getCupEntry().firstWon() == homefirst)) ? woncolor : humancolor :
		(played && (!m.getCupEntry().firstWon() == homefirst)) ? woncolor : Common::Color::White;

	labels.push_back(scr.addLabel(m.getTeam(0)->getName().c_str(), xp - 0.04f, yp,
			TextAlignment::MiddleRight, fontsize, textColor1));
	labels.push_back(scr.addLabel(m.getTeam(1)->getName().c_str(), xp + 0.04f, yp,
			TextAlignment::MiddleLeft, fontsize, textColor2));
	if(played) {
		addResultLabels(m.getResult().HomeGoals, m.getResult().AwayGoals, xp, yp, fontsize, scr, labels);
		if((m.getResult().HomePenalties || m.getResult().AwayPenalties) && rowsAvailable) {
			yp += 0.03f;
			ret += 0.03f;
			addResultLabels(m.getResult().HomePenalties, m.getResult().AwayPenalties, xp, yp, fontsize, scr, labels, " p.");
		}
		auto& c = m.getCupEntry();
		if((c.numMatchesPlayed() > 1) && rowsAvailable) {
			yp += 0.03f;
			ret += 0.03f;
			auto agg = c.aggregate();
			addResultLabels(homefirst ? agg.first : agg.second,
				homefirst ? agg.second : agg.first, xp, yp, fontsize, scr, labels, " agg.");
		}
	} else {
		labels.push_back(scr.addLabel(" - ", xp, yp, TextAlignment::Centered, fontsize, Common::Color::White));
	}

	return ret;
}

float CompetitionScreen::addMatchLabels(const Match& m, float xp, float yp)
{
	return addMatchLabels(m, xp, yp, 0.6f, *this, mResultLabels, true);
}

void CompetitionScreen::drawInfo(bool drewtable)
{
	for(auto l : mResultLabels) {
		removeButton(l);
	}
	mResultLabels.clear();

	float y = 0.1f;
	for(auto m : mRoundMatches) {
		// next round
		y += addMatchLabels(*m, 0.75f, y);
		y += 0.03f;
	}

	if(!(mOneRound && allRoundMatchesPlayed())) {
		// next match
		boost::shared_ptr<Match> m = mCompetition->getNextMatch();
		if(m) {
			addMatchLabels(*m, 0.50f, 0.88f);
		}
	}
}

void CompetitionScreen::updateRoundMatches()
{
	mRoundMatches = mCompetition->getCurrentRoundMatches();
	if(mNextRoundButton)
		mNextRoundButton->hide();
	mSkipButton->show();
	mResultButton->show();
	mMatchButton->show();
}

bool CompetitionScreen::playNextMatch(bool display)
{
	const boost::shared_ptr<Match> m = mCompetition->getNextMatch();
	if(display) {
		mScreenManager->addScreen(boost::shared_ptr<Screen>(new TeamTacticsScreen(mScreenManager, *m,
						[&](Match& mp) -> void {
							RunningMatch rm = RunningMatch(mp);
							MatchResult r;
							while(!rm.matchFinished(&r)) {
								sleep(1);
								mScreenManager->drawScreen();
							}
							if(r.Played) {
								mp.setResult(r);
								mCompetition->matchPlayed(r);
							}
							mScreenManager->dropScreen();
							updateScreenElements();
						})));
		return false;
	}
	else {
		if(m->getTeam(0)->getController().HumanControlled ||
				m->getTeam(1)->getController().HumanControlled) {
			mScreenManager->addScreen(boost::shared_ptr<Screen>(new TeamTacticsScreen(mScreenManager, *m,
							[&](Match& mt) -> void {
							MatchResult r = mt.play(false);
							if(r.Played) {
								mt.setResult(r);
								mCompetition->matchPlayed(r);
							}
							mScreenManager->dropScreen();
							updateScreenElements();
							})));
			return false;
		}
		else {
			MatchResult r = m->play(false);
			if(r.Played) {
				m->setResult(r);
				mCompetition->matchPlayed(r);
				return !mCompetition->getNextMatch();
			} else {
				return false;
			}
		}
	}
}

void CompetitionScreen::updateNextRoundButton()
{
	mSkipButton->hide();
	mResultButton->hide();
	mMatchButton->hide();
	if(mNextRoundButton && !mCompetition->getCurrentRoundMatches().empty())
		mNextRoundButton->show();
}

void CompetitionScreen::updateScreenElements()
{
	const boost::shared_ptr<Match> m = mCompetition->getNextMatch();

	if(allRoundMatchesPlayed()) {
		updateNextRoundButton();
	} else if(!m) {
		mSkipButton->hide();
		mResultButton->hide();
		mMatchButton->hide();
		if(mNextRoundButton)
			mNextRoundButton->hide();
	}
	else {
		mSkipButton->hide();
		if(shouldShowSkipButton())
			mSkipButton->show();
	}
	bool t = drawTable();
	drawInfo(t);
}

bool CompetitionScreen::shouldShowSkipButton() const
{
	const boost::shared_ptr<Match> m = mCompetition->getNextMatch();
	return m && !(m->getTeam(0)->getController().HumanControlled ||
				m->getTeam(1)->getController().HumanControlled);
}

void CompetitionScreen::skipMatches()
{
	while(shouldShowSkipButton()) {
		bool done = playNextMatch(false);
		if(done)
			break;
		if(mOneRound && allRoundMatchesPlayed())
			break;
		updateRoundMatches();
	}
}

void CompetitionScreen::nextRound()
{
	updateRoundMatches();
	updateScreenElements();
}

void CompetitionScreen::skip()
{
	skipMatches();
	updateScreenElements();
}

void CompetitionScreen::result()
{
	playNextMatch(false);
	updateScreenElements();
}

void CompetitionScreen::match()
{
	playNextMatch(true);
}

void CompetitionScreen::buttonPressed(boost::shared_ptr<Button> button)
{
	const std::string& buttonText = button->getText();
	if(buttonText == "Back") {
		if(mOneRound) {
			mScreenManager->dropScreen();
			if(!allRoundMatchesPlayed())
				skipMatches();
		} else
			mScreenManager->dropScreensUntil("Main Menu");
	}
	else if(buttonText == "Save") {
		saveCompetition();
	}
	else if(buttonText == "Next Round") {
		nextRound();
	}
	else if(buttonText == "Skip") {
		skip();
	}
	else if(buttonText == "Result") {
		result();
	}
	else if(buttonText == "Match") {
		match();
	}
}

const std::string& CompetitionScreen::getName() const
{
	return mScreenName;
}

bool CompetitionScreen::allRoundMatchesPlayed() const
{
	for(auto m : mRoundMatches) {
		if(!m->getResult().Played) {
			return false;
		}
	}
	return true;
}

void CompetitionScreen::saveCompetition() const
{
	std::string filename(Menu::getSaveDir());
	filename += "/" + mCompetitionName + ".sav";
	std::ofstream ofs(filename, std::ios::out | std::ios::binary | std::ios::trunc);
	boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
	out.push(boost::iostreams::bzip2_compressor());
	out.push(ofs);
	boost::archive::binary_oarchive oa(out);
	saveCompetition(oa);
	std::cout << "Saved to " << filename << "\n";
}

}


