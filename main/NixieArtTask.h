#pragma once

#include "ArtTask.h"

class NixieArtTask: public ArtTask {
private:
    unsigned updated;
    void update_();
    void update();

protected:
    void run() override;

public:
    NixieArtTask(
	SPI::Bus const		(&spiBus)[2],
	std::function<float()>	getLux,
	KeyValueBroker &	keyValueBroker);

    ~NixieArtTask() override;
};
