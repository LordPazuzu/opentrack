#include "ftnoir_tracker_hydra.h"
#include "opentrack/plugin-api.hpp"

TrackerControls::TrackerControls()
{
	ui.setupUi( this );

	// Connect Qt signals to member-functions
    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(doOK()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(doCancel()));
}

void TrackerControls::doOK() {
    s.b->save();
	this->close();
}

void TrackerControls::doCancel() {
    s.b->reload();
    close();
}

