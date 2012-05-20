#include "match/Math.h"
#include "match/Ball.h"
#include "match/Match.h"
#include "match/Player.h"
#include "match/Team.h"

static const float collisionIgnoreDistance = 1.5f;

Ball::Ball(Match* match)
	: MatchEntity(match, Vector3(0, 0, 0)),
	mGrabbed(false),
	mGrabber(nullptr)
{
	mPosition.v.z = 0.10f;
}

void Ball::update(float time)
{
	if(!mGrabbed) {
		bool outsideBefore1 = mPosition.v.x < -3.96f;
		bool outsideBefore2 = mPosition.v.x > 3.96f;
		MatchEntity::update(time);
		bool outsideAfter1 = mPosition.v.x < -3.36f;
		bool outsideAfter2 = mPosition.v.x > 3.36f;

		if(mVelocity.v.length() > 2.0f &&
				(mPosition.v - mCollisionFreePoint.v).length() > collisionIgnoreDistance) {
			for(int i = 0; i < 2; i++) {
				for(auto p : mMatch->getTeam(i)->getPlayers()) {
					checkCollision(*p);
				}
			}
		}
		if(mPosition.v.z < 0.15f) {
			if(fabs(mVelocity.v.z) < 0.1f)
				mVelocity.v.z = 0.0f;

			if(mVelocity.v.z < -0.1f) {
				// bounciness
				mVelocity.v.z *= -0.65f;
			}
			else {
				mVelocity.v *= 1.0f - time * mMatch->getRollInertiaFactor();
			}
		}
		else {
			mVelocity.v *= 1.0f - time * mMatch->getAirViscosityFactor();
			mVelocity.v.z -= 9.81f * time;
		}

		// net
		mPosition.v.y = clamp(-mMatch->getPitchHeight() * 0.5f - 3.0f,
				mPosition.v.y,
				mMatch->getPitchHeight() * 0.5f + 3.0f);
		if(mPosition.v.z < 2.44f) {
			/* TODO: currently the ball goes through the goal "ceiling".
			 * Add a check that removes ball Z velocity if the ceiling is hit. */
			if(mPosition.v.y > mMatch->getPitchHeight() * 0.5f || mPosition.v.y < -mMatch->getPitchHeight() * 0.5f) {
				if(outsideBefore1 != outsideAfter1 || outsideBefore2 != outsideAfter2) {
					mVelocity.v.zero();
				}
			}
			else if(mPosition.v.y > mMatch->getPitchHeight() * 0.5f - 0.5f ||
					mPosition.v.y < -mMatch->getPitchHeight() * 0.5f + 0.5f) {
				if((fabs(mPosition.v.x) < GOAL_WIDTH_2 + 0.03f &&
							fabs(mPosition.v.x) > GOAL_WIDTH_2 - 0.03f) ||
					       (mPosition.v.z < GOAL_HEIGHT + 0.03f && mPosition.v.z > GOAL_HEIGHT - 0.03f)) {
					// post/bar
					mVelocity.v.y = -mVelocity.v.y;
					mVelocity.v *= 0.9f;
				}
			}
		}
	}
	else {
		mAcceleration = AbsVector3();
		mVelocity = mGrabber->getVelocity();
		mPosition = mGrabber->getPosition();
	}
}

void Ball::kicked(Player* p)
{
	if(mGrabbed && mGrabber != p)
		return;
	mCollisionFreePoint = mPosition;
	mGrabbed = false;
	mGrabber = nullptr;
}

void Ball::checkCollision(const Player& p)
{
	float dist = MatchEntity::distanceBetween(*this, p);
	if(dist < 1.0f) {
		bool catchSuccessful = getVelocity().v.length() / 80.0f < p.getSkills().BallControl;
		if(catchSuccessful)
			mVelocity.v *= -0.1f;
		else
			mVelocity.v *= -0.7f;
	}
}

bool Ball::grabbed() const
{
	return mGrabbed;
}

void Ball::grab(Player* p)
{
	mGrabbed = true;
	mAcceleration = AbsVector3();
	mVelocity = AbsVector3();
	mGrabber = p;
	mPosition = mGrabber->getPosition();
	mPosition.v.z = 0.10f;
}

const Player* Ball::getGrabber() const
{
	return mGrabber;
}


