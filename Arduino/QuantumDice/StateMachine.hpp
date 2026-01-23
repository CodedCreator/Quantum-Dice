#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "IMUhelpers.hpp"

#include <Arduino.h>
#include <cstdint>
#include <map>
#include <optional>

#define FSM_UPDATE_INTERVAL 0  // Update interval in milliseconds
#define MAC_ADDRESS_LENGTH 6

enum class DiceStates : uint8_t;
enum class DiceNumbers : uint8_t;
enum class MeasuredAxises : uint8_t;
enum class UpSide : uint8_t;

class StateMachine; // Forward declaration

constexpr unsigned int IDLETIME         = 3000; // 5000 ms-en
constexpr unsigned int SHOWNEWSTATETIME = 1000; // ms-en to show when new state is initated
constexpr unsigned int MAXENTANGLEDWAITTIME
  = 120000; // ms-en wait for throw in entangled wait, befor return to intitSingle state
constexpr unsigned int STABTIME
  = 200; // ms-en to stabilize after measurement
         // #define WAITTOTHROW 1000            //minumum time it stays in wait to trow

enum class Mode : uint8_t {
    CLASSIC,
    QUANTUM,
    LOW_BATTERY,
};

enum class ThrowState : uint8_t {
    IDLE,
    THROWING,
    OBSERVED,
};

enum class EntanglementState : uint8_t {
    PURE,
    ENTANGLE_REQUESTED,
    ENTANGLED,
    POST_ENTANGLEMENT, // State after entanglement, indicating that the entanglement partner has
                       // been rolled and that we need to roll opposite if in the same measurement
                       // basis
    TELEPORTED,        // State after receiving a teleported observed state - must show that value
                       // if measured on same axis, otherwise random
};

struct State {
  public:
    Mode              mode;
    ThrowState        throwState;
    EntanglementState entanglementState;

    // Comparison operators needed for std::map
    auto operator<(const State &other) const -> bool {
        if (mode != other.mode) {
            return mode < other.mode;
        }
        if (throwState != other.throwState) {
            return throwState < other.throwState;
        }
        return entanglementState < other.entanglementState;
    }

    auto operator==(const State &other) const -> bool {
        return mode == other.mode && throwState == other.throwState
               && entanglementState == other.entanglementState;
    }
};

enum class Trigger : uint8_t {
    // User triggers
    BUTTON_PRESSED,
    // Dice motion triggers
    START_ROLLING,
    STOP_ROLLING,
    // Entanglement triggers
    CLOSE_BY,
    ENTANGLE_REQUEST,
    ENTANGLE_CONFIRM,
    ENTANGLE_STOP,
    MEASUREMENT_RECEIVED,
    // Teleportation triggers
    TELEPORT_INITIATED,
    TELEPORT_CONFIRMED,
    TELEPORT_RECEIVED,
    // Measurement triggers
    MEASURE,
    MEASURE_FAIL,
    // System triggers
    TIMED,
    LOW_BATTERY,
};

struct StateTransition {
    // Dice Mode
    std::optional<Mode> currentMode;
    std::optional<Mode> nextMode;

    // Throw State
    std::optional<ThrowState> currentThrowState;
    std::optional<ThrowState> nextThrowState;

    // Entanglement State
    std::optional<EntanglementState> currentEntanglementState;
    std::optional<EntanglementState> nextEntanglementState;

    // Trigger
    Trigger trigger;
};

void setInitialState();
void printStateName(const char *objectName, State state);

class StateMachine {
  public:
    StateMachine();
    void begin(); // New function to initialize the state machine
    void changeState(Trigger trigger);
    void update();

    void setImuSensor(IMUSensor *imuSensor) {
        _imuSensor = imuSensor;
    }

    [[nodiscard]]
    auto getCurrentState() const -> State {
        return currentState;
    }

    auto getStateTransition(State currentState, Trigger trigger) -> StateTransition;

  private:
    // State handlers for each mode/throwState/entanglementState combination
    void enterClassicIdle();
    void whileClassicIdle();

    void enterQuantumIdle();
    void whileQuantumIdle();

    void enterThrowing();
    void whileThrowing();

    void enterObserved();
    void whileObserved();

    void enterLowBattery();
    void whileLowBattery();

    // Communication functions
    static void sendWatchDog();
    static void sendMeasurements(uint8_t *target, State state, DiceNumbers diceNumber,
                                 UpSide upSide, MeasuredAxises measureAxis);
    static void sendEntangleRequest(uint8_t *target);
    static void sendEntanglementConfirm(uint8_t *target);
    static void sendEntangleDenied(uint8_t *target);
    static void sendTeleportRequest(uint8_t *target_m, uint8_t *target_b);
    static void sendTeleportConfirm(uint8_t *target);
    static void sendTeleportPayload(uint8_t *target, State state, DiceNumbers diceNumber,
                                    UpSide upSide, MeasuredAxises measureAxis,
                                    uint8_t *entangled_peer);
    static void sendTeleportPartner(uint8_t *target_n, uint8_t *new_partner_b);

    IMUSensor *_imuSensor;
    State      currentState;
    uint8_t    current_peer[MAC_ADDRESS_LENGTH];
    uint8_t    next_peer[MAC_ADDRESS_LENGTH];

    unsigned long stateEntryTime;

    // EntangStateMachine entangStateMachine;

    struct StateFunction {
        void (StateMachine::*onEntry)();
        void (StateMachine::*whileInState)();
    };

    static const std::map<State, StateFunction>  stateFunctions;
    static const std::array<StateTransition, 36> stateTransitions;

    // Partner's measurement info (for post-entanglement state)
    MeasuredAxises partnerMeasurementAxis;
    DiceNumbers    partnerDiceNumber;

    // Teleported measurement info (for teleported state)
    MeasuredAxises teleportedMeasurementAxis;
    DiceNumbers    teleportedDiceNumber;
};

#endif // STATEMACHINE_H
