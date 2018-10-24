#include "GlobalVariable.h"

std::vector<std::shared_ptr<QSemaphore>> GlobalVariable::s_semFilterFrees;
std::vector<std::shared_ptr<QSemaphore>> GlobalVariable::s_semFilterUseds;
std::vector<QList<std::shared_ptr<GlobalVariable::FrameDecoded>>> GlobalVariable::s_aryProducedFrameses(threads);

std::vector<std::shared_ptr<QSemaphore>> GlobalVariable::s_semPreProFrees;
std::vector<std::shared_ptr<QSemaphore>> GlobalVariable::s_semPreProUseds;
std::vector<QList<GlobalVariable::FramePreProcessed>> GlobalVariable::s_aryPreProcessedFramesess(threads);
GlobalVariable::GlobalVariable()
{

}


GlobalVariable::~GlobalVariable()
{
}
