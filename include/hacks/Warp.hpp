#pragma once
class INetMessage;
namespace hacks::tf2::warp
{
void SendNetMessage(INetMessage &msg);
void CL_SendMove_hook();
} // namespace hacks::tf2::warp
