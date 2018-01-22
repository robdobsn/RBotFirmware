call particle compile photon --target 0.8.0-rc.1
for %%x in (*.bin) do (
        call particle flash --usb %%x
        goto breakout
    )
:breakout
for %%x in (*.bin) do del %%x
