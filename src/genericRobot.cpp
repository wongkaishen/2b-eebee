#include <cstdint>
#include <cmath>
#include <random>
#include <string>
#include <vector>

#include "genericRobot.h"
#include "abstractRobot/robot.h"
#include "logger.h"
#include "upgrades/upgrades.h"
#include "vector2d.h"
#include "environment.h"

using namespace std;


GenericRobot::GenericRobot(
    RobotParameter robotParam,
    Environment* env,
    uint_fast64_t rngSeed,
    Logger* logger
) {
    this->position = robotParam.position;
    this->name = robotParam.name;
    this->symbol = robotParam.symbol;

    this->environment = env;
    this->logger = logger;

    // A seed initialize the random number generator (RNG)
    // The advantage is that if we gave it the same seed
    // it will always generate the same sequence of random numbers
    this->rng = mt19937_64(rngSeed);
}

DeadState GenericRobot::die() {
    if (respawnCountLeft == 0) {
        selfLog("Robot " + name + " has died and cannot respawn anymore.");
        return DeadState::Dead;
    }

    respawnCountLeft--;
    return DeadState::Respawn;
}

void GenericRobot::thinkAndExecute() {
    int maxFireDistance = getMaxFiringDistance();
    int bulletsPerShot = getBulletsPerShot();

    // Generate x and y between -1, 0, or 1
    // Note that we only generate integers here
    uniform_int_distribution<int> next_x_generator(this->movementRange[0], this->movementRange[1]);
    uniform_int_distribution<int> next_y_generator(this->movementRange[0],this->movementRange[1]);

    // Will be used for look() and move()
    int next_x = 0;
    int next_y = 0;

    bool validLookCenter = false;
    while (!validLookCenter) {
        next_x = next_x_generator(rng);
        next_y = next_y_generator(rng);

        // Center of vision must be within bounds
        // simply for efficiency
        validLookCenter = environment->isWithinBounds(Vector2D(next_x, next_y));
    }

    vector<Vector2D> lookResult = look(next_x, next_y);

    for (const Vector2D &pos : lookResult) {
        selfLog("Robot found at: ("+ to_string(pos.x)+ ", " + to_string(pos.y) + ")");
        int distance = calcDistance(pos);
        if (distance > maxFireDistance)
            continue;

        for (int i = 0; i < bulletsPerShot; i++) {
            selfLog("Attemting to fire at: (" + to_string(pos.x)+ ", " + to_string(pos.y) + ")");
            fire(pos.x, pos.y);
        }
    }

    bool validMovement = false;

    // Keep generating next movement
    // until a valid one is found
    while (!validMovement) {
        next_x = next_x_generator(rng);
        next_y = next_y_generator(rng);

        validMovement = environment->isPositionAvailable(
            this->position + Vector2D(next_x, next_y)
        );
    }

    move(next_x, next_y);
}

vector<Vector2D> GenericRobot::look(int x, int y) {
    vector<Vector2D> lookResult = {};
    selfLog("Looking.");
    // Loop through a 3x3 square around center
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {

            // Center of look coordinate + offset
            Vector2D relativePositionToLook = Vector2D(x, y) + Vector2D(i, j);

            // Same position, but from the whole grid point of view
            Vector2D absolutePositionToLook = this->position + relativePositionToLook;

            // If no robots, go to next cycle
            if (!environment->isRobotHere(absolutePositionToLook))
                continue;                                           

            GenericRobot* seenRobot = environment->getRobotAtPosition(absolutePositionToLook);
            // If you are looking at urself, skip
            if (seenRobot == this)
                continue;

            // If robot is found, add their coordinates to the lookResult
            lookResult.push_back(relativePositionToLook);
        }
    }

    return lookResult;
}

void GenericRobot::fire(int x, int y) {
    Vector2D target(x, y);
    Vector2D targetAbsolutePosition = position + target;

    GenericRobot* targetRobot = environment->getRobotAtPosition(targetAbsolutePosition);
    selfLog("Fired at " + to_string(targetAbsolutePosition.x) + ", " + to_string(targetAbsolutePosition.y));
    // call die() directly
    // Allow flexibility of 'killing' the oponent later since we can set the probability
    if(randomBool(0.7)) {
        DeadState deadState = targetRobot->die();
        selfLog("Killed " + targetRobot->getName() + " at " + to_string(targetAbsolutePosition.x) + ", " + to_string(targetAbsolutePosition.y));
        environment->notifyKill(this, targetRobot, deadState);
    }
    else {
        selfLog("Missed " + targetRobot->getName() + " at " + to_string(targetAbsolutePosition.x) + ", " + to_string(targetAbsolutePosition.y));
    }
    shellCount--;
}

int GenericRobot::getBulletsPerShot() const {
    return 1;
}

int GenericRobot::getMaxFiringDistance() const {
    return 1;
}

void GenericRobot::move(int x, int y) {
    position += Vector2D(x,y);
    selfLog("Moved to " + to_string(position.x) + ", " + to_string(position.y));
}

string GenericRobot::getName() const {
    return this->name;
}

char GenericRobot::getSymbol() const {
    return this->symbol;
}

Vector2D GenericRobot::getPosition() const {
    return position;
}

// No upgrades, so return empty vector
const vector<Upgrade>& GenericRobot::getUpgrades() const {
    return upgrades;
}


const vector<Upgrade>& GenericRobot::getPendingUpgrades() const {
    return pendingUpgrades;
}

void GenericRobot::setShellCount(int newShellCount){
    shellCount = newShellCount;
}

bool GenericRobot::randomBool(double probability) {
    uniform_real_distribution<double> dist(0.0, 1.0);

    // Generate a number between 0 and 1, and check if it's below probability
    // if yes (which depends on the value of probability) return true
    // else return false
    double num = dist(this->rng);
    return num < probability;
}

void GenericRobot::selfLog(const string& msg) {
    this->logger->log(
        name + ": " + msg
    );
}

// A crude way to give the distance based on 'square area'
int GenericRobot::calcDistance(Vector2D a) const {
    // Return the bigger difference, either x or y
    return max(abs(a.x), abs(a.y));
}

UpgradeState GenericRobot::chosenForUpgrade() {
    // Check for the current upgrade and pending upgrade count
    // Stop if already enough
    if (upgrades.size() + pendingUpgrades.size() == 3)
        return UpgradeFull;

    // Select track within the possible upgrade tracks
    uniform_int_distribution<int> trackIndexGen(0, possibleUpgradeTrack.size() - 1);
    int chosenTrackIndex = trackIndexGen(rng);
    UpgradeTrack chosenTrack = possibleUpgradeTrack[chosenTrackIndex];

    // List out all upgrades under a track
    vector<Upgrade> upgradesToChoose = getUpgradesUnderTrack(chosenTrack);

    // Select upgrades under the track
    uniform_int_distribution<int> upgradeIndexGen(0, upgradesToChoose.size() - 1);
    int chosenUpgradeIndex = upgradeIndexGen(rng);
    Upgrade chosenUpgrade = upgradesToChoose[chosenUpgradeIndex];

    // Store chosen upgrade for the next upgrade cycle
    pendingUpgrades.push_back(chosenUpgrade);

    return AvailableForUpgrade;
}
