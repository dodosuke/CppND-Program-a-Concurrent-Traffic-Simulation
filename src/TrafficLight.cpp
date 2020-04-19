#include <iostream>
#include <random>
#include <future>
#include <queue>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this] { return !_messages.empty(); }); // pass unique lock to condition variable

    // remove last vector element from queue
    T msg = std::move(_messages.back());
    _messages.pop_back();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);

    // add vector to queue
    _messages.push_back(std::move(msg));
    _cond.notify_one(); // notify client after pushing new Vehicle into vector
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _queue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

TrafficLight::~TrafficLight() {}

void TrafficLight::waitForGreen()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        if (_queue->receive() == TrafficLightPhase::green) { return; }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    std::chrono::time_point<std::chrono::system_clock> lastUpdated = std::chrono::system_clock::now();
    int duration = rand()%2000 + 4000;

    while (true) {
        std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdated).count();

        if (diff > duration) {
            _currentPhase == TrafficLightPhase::red
                ? _currentPhase = TrafficLightPhase::green
                : _currentPhase = TrafficLightPhase::red;

            lastUpdated = now;

            std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _queue, std::move(_currentPhase));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}