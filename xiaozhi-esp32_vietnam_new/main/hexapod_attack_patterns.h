#pragma once

#include <cstdint>
#include <esp_timer.h>
#include "hexapod_servo_controller.h"
#include "hexapod_gait_generator.h"

/**
 * AttackPatterns - High-level attack behavior controller
 *
 * PURPOSE:
 *   Manages attack animations (Strike, Lunge) for the hexapod robot.
 *   Works alongside GaitGenerator but takes over servo control during attacks.
 *
 * ARCHITECTURE:
 *   - Receives attack commands: ExecuteAttack(type, intensity)
 *   - Pauses GaitGenerator during attack
 *   - Computes per-frame servo angles using attack animation headers
 *   - Releases control back to GaitGenerator when done
 *   - Called by main loop via UpdateFrame()
 *
 * INTERACTION WITH GAIT GENERATOR:
 *   GaitGenerator manages normal locomotion (walk, stop)
 *   AttackPatterns temporarily OVERRIDES servo control during attacks
 *   When attack finishes, GaitGenerator resumes
 *
 * USAGE:
 *   1. Initialize: attack.Initialize(gait_gen, servo_ctrl)
 *   2. Execute:    attack.ExecuteAttack(STRIKE, 100)
 *   3. Update:     attack.UpdateFrame()  // Call every 50ms
 *   4. Check:      if (!attack.IsAttacking()) { ... }
 */
class AttackPatterns {
public:
    // Supported attack types
    enum AttackType {
        STRIKE,   // Quick jab with front legs (600ms)
        LUNGE,    // Forward charge attack (1100ms)
    };

    // Singleton access
    static AttackPatterns& GetInstance();

    // ============ LIFECYCLE ============

    /**
     * Initialize attack patterns with required dependencies.
     *
     * @param gait_gen Pointer to GaitGenerator (to pause/resume during attacks)
     * @param servo_ctrl Pointer to ServoController (to command servos)
     * @return true if initialization successful
     *
     * WHY TWO DEPENDENCIES?
     *   servo_ctrl: We need direct servo control to execute attack animations
     *   gait_gen: We need to pause walking when attack starts,
     *             then resume when attack finishes
     */
    bool Initialize(GaitGenerator* gait_gen, ServoController* servo_ctrl);

    /**
     * Cleanup and shutdown attack patterns controller.
     */
    void Shutdown();

    // ============ ATTACK COMMANDS ============

    /**
     * Execute an attack with given intensity.
     *
     * @param attack Attack type (STRIKE or LUNGE)
     * @param intensity Attack strength: 0-100
     *
     * BEHAVIOR:
     *   1. If already attacking, cancel current attack first
     *   2. Pause GaitGenerator (stops walking motion)
     *   3. Record attack start time
     *   4. Set attack state to active
     *   5. UpdateFrame() will drive the animation each cycle
     *
     * WHY PAUSE GAIT?
     *   Walking motion uses all 18 servos
     *   Attack animation also uses all 18 servos
     *   Running both simultaneously would cause conflicts
     *   Safest approach: pause walk, execute attack, resume walk
     */
    void ExecuteAttack(AttackType attack, uint8_t intensity);

    /**
     * Stop current attack immediately.
     * Return to neutral position, resume walking if it was active.
     *
     * WHY NEEDED?
     *   Safety: Allow immediate stop if robot is in danger
     *   Testing: Interrupt attacks during development
     */
    void StopAttack();

    /**
     * Cancel current attack (alias for StopAttack).
     * For semantic clarity in code.
     */
    void CancelAttack() { StopAttack(); }

    // ============ STATUS ============

    /**
     * Is the robot currently executing an attack?
     * @return true if attack is active
     * WHY: Prevent issuing new commands while attack in progress
     */
    bool IsAttacking() const { return is_attacking_; }

    /**
     * Get the current attack type being executed.
     * @return Current AttackType
     */
    AttackType GetCurrentAttack() const { return current_attack_; }

    /**
     * Get elapsed time since attack started.
     * @return Milliseconds since attack began
     * WHY: For UI feedback, logging, debugging
     */
    uint32_t GetAttackElapsedMs() const;

    // ============ MAIN UPDATE ============

    /**
     * Update attack animation by one frame.
     * MUST be called periodically (~20Hz = every 50ms).
     *
     * ALGORITHM:
     *   1. If not attacking, return immediately
     *   2. Compute elapsed time since attack started
     *   3. Check if attack duration has expired → auto-complete
     *   4. Compute servo angles for current phase (calls attack-specific function)
     *   5. Send angles to ServoController
     *
     * WHY 20Hz?
     *   Servo response time ~20ms
     *   20Hz = 50ms between updates = smooth enough for visual quality
     *   Higher frequency wastes I2C bandwidth
     */
    void UpdateFrame();

private:
    // Private constructor (singleton pattern)
    AttackPatterns();

    // ============ INTERNAL HELPERS ============

    /**
     * Complete the current attack (called when duration expires).
     * Returns all servos to neutral, resumes GaitGenerator.
     *
     * WHY SEPARATE FUNCTION?
     *   Same completion logic whether attack timed out or was cancelled
     *   Centralizes cleanup code
     */
    void CompleteAttack();

    /**
     * Compute servo angles for STRIKE attack at current elapsed time.
     * Delegates to StrikeAttack::ComputeStrikeFrame()
     *
     * @param elapsed_ms Time since attack started
     * @param angles Output: all 18 servo angles
     */
    void ComputeStrikeFrame(uint32_t elapsed_ms, float angles[18]);

    /**
     * Compute servo angles for LUNGE attack at current elapsed time.
     * Delegates to LungeAttack::ComputeLungeFrame()
     *
     * @param elapsed_ms Time since attack started
     * @param angles Output: all 18 servo angles
     */
    void ComputeLungeFrame(uint32_t elapsed_ms, float angles[18]);

    // ============ MEMBERS ============

    // Dependencies (not owned, just referenced)
    ServoController* servo_controller_;
    GaitGenerator* gait_generator_;

    // Attack state
    AttackType current_attack_;
    uint32_t attack_start_time_ms_;  // esp_timer_get_time() / 1000
    uint8_t attack_intensity_;       // 0-100
    uint32_t attack_duration_ms_;    // Total duration for current attack type

    // State flags
    bool is_attacking_;
    bool was_walking_before_;  // Was GaitGenerator walking before attack started?
};
