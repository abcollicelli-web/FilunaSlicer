#pragma once
// Filuna first-version native updater: strict state progression only.
// Does NOT download, replace executables, kill processes, or claim verification.
// Inputs named "verified" MUST come from independent byte-bound verifiers.
#include "NativeUpdatePolicy.hpp"
#include <string>

namespace Filuna { namespace Update {
enum class Phase {
    Idle, ConsentRequired, DownloadAllowed, StagedVerified,
    CloseReady, HelperPending, InstalledConfirmed, RollbackRequired, Blocked
};

class NativeUpdateFlow {
public:
    Phase phase() const { return phase_; }
    const char* reason() const { return reason_; }
    Outcome action() const { return decision_.outcome; }
    const Release& release() const { return release_; }

    bool offer(const Installed& installed, const Release& release, const TrustEvidence& trust) {
        // Repeated, overlapping or downgraded offers cannot replace an active transaction.
        if (phase_ != Phase::Idle && phase_ != Phase::Blocked)
            return fail_without_mutating("UPDATE_ALREADY_ACTIVE");
        const Decision decision = decide(installed, release, trust);
        if (decision.outcome == Outcome::Block || decision.outcome == Outcome::NoUpdate) {
            phase_ = Phase::Blocked;
            reason_ = decision.code;
            return false;
        }
        installed_ = installed;
        release_ = release;
        trust_ = trust;
        decision_ = decision;
        phase_ = is_binary() ? Phase::ConsentRequired : Phase::DownloadAllowed;
        reason_ = "OFFER_ACCEPTED";
        return true;
    }
    bool consent(bool explicitly_approved) {
        if (phase_ != Phase::ConsentRequired)
            return fail_without_mutating("CONSENT_NOT_DUE");
        if (!explicitly_approved) {
            phase_ = Phase::Blocked;
            reason_ = "UPDATE_DECLINED";
            return false;
        }
        phase_ = Phase::DownloadAllowed;
        reason_ = "CONSENT_CONFIRMED";
        return true;
    }
    bool stage_verified_payload(bool exact_bytes_sha256_verified,
                                bool manifest_and_code_signatures_verified,
                                bool source_and_qa_still_authorized) {
        if (phase_ != Phase::DownloadAllowed)
            return fail_without_mutating("DOWNLOAD_NOT_AUTHORIZED");
        if (!exact_bytes_sha256_verified || !manifest_and_code_signatures_verified ||
            !source_and_qa_still_authorized) {
            phase_ = Phase::Blocked;
            reason_ = "STAGING_VERIFICATION_FAILED";
            return false;
        }
        phase_ = Phase::StagedVerified;
        reason_ = "STAGED_VERIFIED";
        return true;
    }
    // For C/D, the caller MUST re-query the actual editor state immediately
    // before closing; earlier UI consent never authorizes killing a running job.
    bool ready_for_controlled_close(bool all_documents_saved, bool no_running_jobs,
                                    bool reliable_rollback_snapshot) {
        if (!is_binary() || phase_ != Phase::StagedVerified)
            return fail_without_mutating("BINARY_STAGE_NOT_READY");
        if (!all_documents_saved || !no_running_jobs || !reliable_rollback_snapshot)
            return fail_without_mutating("SAVE_TASK_OR_ROLLBACK_REQUIRED");
        phase_ = Phase::CloseReady;
        reason_ = "CONTROLLED_CLOSE_ALLOWED";
        return true;
    }
    bool helper_handoff(bool helper_signature_and_path_verified) {
        if (!is_binary() || phase_ != Phase::CloseReady)
            return fail_without_mutating("HELPER_NOT_READY");
        if (!helper_signature_and_path_verified) {
            phase_ = Phase::Blocked;
            reason_ = "HELPER_SIGNATURE_FAILED";
            return false;
        }
        phase_ = Phase::HelperPending;
        reason_ = "WAITING_FOR_SIGNED_RECEIPT_AND_RELAUNCH";
        return true;
    }
    bool receipt_after_relaunch(const Version& actual_version,
                                bool verified_signed_helper_receipt,
                                bool installed_binary_hash_verified,
                                bool application_reopened_and_healthy) {
        if (!is_binary() || phase_ != Phase::HelperPending)
            return fail_without_mutating("NO_PENDING_HELPER");
        if (!verified_signed_helper_receipt || !installed_binary_hash_verified ||
            !application_reopened_and_healthy || !(actual_version == release_.version)) {
            phase_ = Phase::RollbackRequired;
            reason_ = "ROLLBACK_REQUIRED_NO_VALID_RELAUNCH_ACK";
            return false;
        }
        phase_ = Phase::InstalledConfirmed;
        reason_ = "VERIFIED_RELAUNCH_ACK";
        return true;
    }
    // For A/B, changing data/module state is outside this orchestrator.
    bool data_or_module_activation_ack(bool verified_on_disk_and_activated) {
        if (is_binary() || phase_ != Phase::StagedVerified)
            return fail_without_mutating("DATA_STAGE_NOT_READY");
        if (!verified_on_disk_and_activated) {
            phase_ = Phase::RollbackRequired;
            reason_ = "DATA_ROLLBACK_REQUIRED";
            return false;
        }
        phase_ = Phase::InstalledConfirmed;
        reason_ = "DATA_ACTIVATION_ACK";
        return true;
    }
    void reset_after_operator_recovery() {
        // Does not itself roll anything back or clear a persistent helper journal.
        phase_ = Phase::Idle;
        reason_ = "RECOVERY_HANDLED_EXTERNALLY";
        decision_ = {Outcome::NoUpdate, "NO_UPDATE"};
        trust_ = {};
        release_ = {};
        installed_ = {};
    }
private:
    bool is_binary() const {
        return decision_.outcome == Outcome::ConfirmRestart ||
               decision_.outcome == Outcome::ConfirmInstaller;
    }
    bool fail_without_mutating(const char* why) {
        last_rejection_ = why;
        return false;
    }
    Phase phase_ = Phase::Idle;
    const char* reason_ = "NO_UPDATE";
    const char* last_rejection_ = "";
    Installed installed_;
    Release release_;
    TrustEvidence trust_;
    Decision decision_{Outcome::NoUpdate, "NO_UPDATE"};
};
}} // namespace Filuna::Update
