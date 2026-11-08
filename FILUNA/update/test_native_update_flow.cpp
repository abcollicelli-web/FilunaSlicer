#include "NativeUpdateFlow.hpp"
#include <cassert>
#include <iostream>
using namespace Filuna::Update;

int main(){
    Installed installed{{1,0,0},"owner-beta","windows","x64","first-base",false,false};
    Release patch{{1,0,1},{1,0,0},{1,0,1},Class::C,"owner-beta","windows",
                  "x64","first-base","Correções e melhorias","",true,false};
    TrustEvidence trust{true,true,true,true,true,true,true};
    NativeUpdateFlow flow;
    // Fail closed without valid manifest, or without a user confirmation.
    TrustEvidence missing=trust;missing.ecdsa_manifest=false;
    assert(!flow.offer(installed,patch,missing)&&flow.phase()==Phase::Blocked);
    flow.reset_after_operator_recovery();
    assert(flow.offer(installed,patch,trust));
    assert(flow.phase()==Phase::ConsentRequired);
    assert(!flow.stage_verified_payload(true,true,true));
    assert(flow.phase()==Phase::ConsentRequired);
    assert(flow.consent(true)&&flow.phase()==Phase::DownloadAllowed);
    assert(!flow.offer(installed,patch,trust)); // a second offer cannot hijack the first
    assert(!flow.stage_verified_payload(true,false,true));
    assert(flow.phase()==Phase::Blocked);
    flow.reset_after_operator_recovery();

    assert(flow.offer(installed,patch,trust));
    assert(flow.consent(true));
    assert(flow.stage_verified_payload(true,true,true));
    assert(!flow.ready_for_controlled_close(false,true,true)); // unsaved
    assert(!flow.ready_for_controlled_close(true,false,true)); // active job
    assert(!flow.ready_for_controlled_close(true,true,false)); // no rollback
    assert(flow.phase()==Phase::StagedVerified);
    assert(flow.ready_for_controlled_close(true,true,true));
    assert(!flow.helper_handoff(false,true,true,true)); // helper cannot be untrusted
    assert(flow.phase()==Phase::Blocked);
    flow.reset_after_operator_recovery();
    assert(flow.offer(installed,patch,trust));
    assert(flow.consent(true));
    assert(flow.stage_verified_payload(true,true,true));
    assert(flow.ready_for_controlled_close(true,true,true));
    assert(flow.helper_handoff(true,true,true,true));
    assert(!flow.receipt_after_relaunch({1,0,0},true,true,true));
    assert(flow.phase()==Phase::RollbackRequired);
    flow.reset_after_operator_recovery();

    assert(flow.offer(installed,patch,trust));
    assert(flow.consent(true));
    assert(flow.stage_verified_payload(true,true,true));
    assert(flow.ready_for_controlled_close(true,true,true));
    // A change after CloseReady revokes the earlier close authorization.
    assert(!flow.helper_handoff(true,false,true,true));
    assert(flow.phase()==Phase::StagedVerified);
    assert(!flow.helper_handoff(true,true,true,true)); // must re-authorize close
    assert(flow.ready_for_controlled_close(true,true,true));
    assert(!flow.helper_handoff(true,true,false,true));
    assert(flow.phase()==Phase::StagedVerified);
    assert(flow.ready_for_controlled_close(true,true,true));
    assert(!flow.helper_handoff(true,true,true,false));
    assert(flow.phase()==Phase::StagedVerified);
    assert(flow.ready_for_controlled_close(true,true,true));
    assert(flow.helper_handoff(true,true,true,true));
    assert(flow.receipt_after_relaunch({1,0,1},true,true,true));
    assert(flow.phase()==Phase::InstalledConfirmed);
    // A/B cannot enter binary helper workflow.
    flow.reset_after_operator_recovery();
    patch.kind=Class::A;patch.restart=false;patch.update_base="";
    trust.authenticode=false;
    assert(flow.offer(installed,patch,trust));
    assert(flow.phase()==Phase::DownloadAllowed);
    assert(!flow.consent(true));
    assert(!flow.ready_for_controlled_close(true,true,true));
    assert(flow.stage_verified_payload(true,true,true));
    assert(flow.data_or_module_activation_ack(true));
    assert(flow.phase()==Phase::InstalledConfirmed);

    flow.reset_after_operator_recovery();
    patch.kind=Class::D;patch.restart=true;patch.installer_necessary=true;
    patch.installer_reason="Engine ABI updated";trust.authenticode=true;
    assert(flow.offer(installed,patch,trust));
    assert(flow.phase()==Phase::ConsentRequired);
    assert(!flow.consent(false)&&flow.phase()==Phase::Blocked);
    std::cout<<"NATIVE_UPDATE_FLOW_PASS\n";
}
