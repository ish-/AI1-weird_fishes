#include "Boid.h"
#include "util.cpp"
#include "raylib.h"
#include "raymath.h"
#define _USE_MATH_DEFINES
#include <math.h>

#include "config.hpp"
#include "Obstacle.h"

vector<int> Boid::scores {0,0,0,0,0,0,0,0,0};

Vector2 nullVec2 = Vector2{0,0};

Boid::Boid(Vector2 _pos, Vector2 _vel, int _group, Texture2D _tex) {
    pos = _pos;
    vel = _vel;
    group = _group;
    tex = _tex;
    texRect = Rectangle{ 0,0, (float)tex.width, (float)tex.height };
}

void Boid::updateClosest(Boid* boid, float& dist, Vector2& dir) {
    if (boid->group == (group + 1) % GROUPS_COUNT) {
        if (dist < FOLLOW_DIST && dist < closestBoid.dist)
            closestBoid = { dist, boid };

            if (dist < size * .9)
                kill(boid);
    }
}

void Boid::kill(Boid* victim) {
    //boidsToDeleteIdxs.push_back(victimIdx);
    victim->hp -= HIT_HP;
    hp += HIT_HP / 2;
    if (victim->hp < 0.) {
        scores[group]++;
        victim->isAlive = false;
    }
    int size = 50;
    DrawRectangleLines(pos.x - size/2, pos.y - size/2, size, size, RED);
}

void Boid::checkBoudariesAndReflect() {
    if (pos.x <= 0 || pos.x >= W) {
        vel.x = -vel.x;
        pos.x = (pos.x <= 0) ? 0 : W;
    }

    if (pos.y <= 0 || pos.y >= H) {
        vel.y = -vel.y;
        pos.y = (pos.y <= 0) ? 0 : H;
    }
}

Vector2 Boid::followClosest() const {
    if (closestBoid.dist < 9000.)
      return  Vector2Normalize(closestBoid.boid->pos - pos);
    return nullVec2;
}

Vector2 Boid::separate(Boid* boid, float& dist, Vector2& dir) const {
    if (boid->group == this->group || boid->group != (GROUPS_COUNT + group - 1) % GROUPS_COUNT) {
        if (dist < SEPARATE_DIST) {
            return dir * -1;
        }
    }
    return nullVec2;
}

Vector2 Boid::seek(Vector2& p, float& dir) const {
    Vector2 to = (p - pos) * dir;
    return to;
}

Vector2 Boid::avoid(Obstacle* obstacle) const {
    Vector2 infl{0,0};
    if (CheckCollisionCircleRec(pos, size, obstacle->boundingRect)) {
        Vector2 to = pos - (obstacle->pos + Vector2{ obstacle->size, obstacle->size } / 2);
        obstacle->touched = min(255., obstacle->touched + 10.);
        return Vector2Normalize(to);
    }
    return nullVec2;
}

Vector2 Boid::align(Boid* boid, float& dist, Vector2& dir) const {
    if (group == boid->group) {
        if (dist < ALIGN_DIST) {
            return boid->vel;
        }
    }
    return nullVec2;
}

void Boid::update(vector<Boid*> boids, vector<Obstacle*> obstacles, Vector2& mouse, float& mouseZ) {
    closestBoid = {9999., nullptr };
    Vector2 avoidInfl{0,0};
    Vector2 alignInfl{0,0};
    Vector2 separateInfl{0,0};
    Vector2 seekInfl = seek(mouse, mouseZ);

    for (Boid* boid : boids) {
        if (boid == this)
            continue;
        
        Vector2 to = boid->pos - pos;
        float dist = Vector2Length(to);
        Vector2 dir = Vector2Normalize(to);

        updateClosest(boid, dist, dir);
        alignInfl += align(boid, dist, dir);
        separateInfl += separate(boid, dist, dir);
    }

    Vector2 followInfl = followClosest();
    
    for (Obstacle* obstacle : obstacles)
        avoidInfl += avoid(obstacle);

    Vector2 infl = Vector2{0,0}
        + Vector2Normalize(avoidInfl) * OBSTACLE_AVOID_MULT
        + Vector2Normalize(alignInfl) * ALIGN_MULT
        + Vector2Normalize(separateInfl) * SEPARATE_MULT
        + Vector2Normalize(seekInfl)
        + Vector2Normalize(followInfl) * FOLLOW_MULT
    ;

    //acc = infl - vel;
    //vel += acc;
    vel += infl * ALL_INFL_MULT;
    vel = Vector2ClampValue(vel, -MAX_SPEED, MAX_SPEED);
    checkBoudariesAndReflect();
    pos += vel;

    if ((hp -= .001) < 0.)
        isAlive = false;
    // draw();
}

void Boid::__update() {
    vel = vel + acc;
    acc = Vector2{ 0, 0 };
    vel = Vector2ClampValue(vel, -MAX_SPEED, MAX_SPEED);
    float noiseAmp = .2;
    vel = vel + Vector2{ randf(-noiseAmp,noiseAmp), randf(-noiseAmp,noiseAmp) };
    checkBoudariesAndReflect();
    pos = pos + vel;
}
void Boid::draw() const {
    float s = (.5 + hp) * size * 4;
    float angle = atan2(vel.y, vel.x) / M_PI * 180 + 180;
    DrawTexturePro(tex, texRect, { pos.x, pos.y, s, s }, { s/2, s/2 }, angle, COLORS[group]);
}
