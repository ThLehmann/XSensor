--[[
    ----------------------------------------------------------------------------
    App für den XSensor ab V1.31 von Thomas Lehmann auf der Jeti DC-16
    ----------------------------------------------------------------------------
	MIT License
	
    Copyright (c) 2017 A. Fromm
   
    Hiermit wird unentgeltlich jeder Person, die eine Kopie der Software und der
    zugehörigen Dokumentationen (die "Software") erhält, die Erlaubnis erteilt,
    sie uneingeschränkt zu nutzen, inklusive und ohne Ausnahme mit dem Recht, sie
    zu verwenden, zu kopieren, zu verändern, zusammenzufügen, zu veröffentlichen,
    zu verbreiten, zu unterlizenzieren und/oder zu verkaufen, und Personen, denen
    diese Software überlassen wird, diese Rechte zu verschaffen, unter den
    folgenden Bedingungen: 
    Der obige Urheberrechtsvermerk und dieser Erlaubnisvermerk sind in allen Kopien
    oder Teilkopien der Software beizulegen. 
    DIE SOFTWARE WIRD OHNE JEDE AUSDRÜCKLICHE ODER IMPLIZIERTE GARANTIE BEREITGESTELLT,
    EINSCHLIEßLICH DER GARANTIE ZUR BENUTZUNG FÜR DEN VORGESEHENEN ODER EINEM
    BESTIMMTEN ZWECK SOWIE JEGLICHER RECHTSVERLETZUNG, JEDOCH NICHT DARAUF BESCHRÄNKT.
    IN KEINEM FALL SIND DIE AUTOREN ODER COPYRIGHTINHABER FÜR JEGLICHEN SCHADEN ODER
    SONSTIGE ANSPRÜCHE HAFTBAR ZU MACHEN, OB INFOLGE DER ERFÜLLUNG EINES VERTRAGES,
    EINES DELIKTES ODER ANDERS IM ZUSAMMENHANG MIT DER SOFTWARE ODER SONSTIGER
    VERWENDUNG DER SOFTWARE ENTSTANDEN. 
	----------------------------------------------------------------------------
	Idea of Fuelstation by ECU data display of Bernd Woköck
	----------------------------------------------------------------------------
	Versionshistory:
	27.01.2018	V0.01	released
	29.01.2018	V0.02	delete not used variables, Tank alarm added
				V3.01	Automation of Tank Announcement
	10.02.2018	V3.02	Änderungen für XSensor V1.31 (läuft auch mit älteren
						Versionen vom XSensor!!!)
	11.02.2018	V3.03	Sensoren reduziert da in der DC-16 nicht ausreichend
						Speicher zur Verfügung steht					
--]]
collectgarbage()
--------------------------------------------------------------------------------
local initAnimation = true
local model, owner, wave = " ", " ", "..."
local Version = "3.03"
local trans, anSw, anGo
local rx1Vtg, rx1Q, rx1A1, rx1A2 = 0, 0, 0, 0
local rx2Vtg, rx2Q, rx2A1, rx2A2 = 0, 0, 0, 0
local playDone, sensorId, sensorPa, alarmValue
local sensid1, sensid2, sensid3, sensid10, sensid11 = "-", "-", "-", "-", 100
local sensid12, sensid13, sensid14, sensid20, sensid21 = "-", "-", "-", "-", "-"
local sensid22, sensid23, sensid24, sensid25, sensid26 = "-", "-", "-", "-", "-"
local sensid30, sensid31, sensid35, sensid40, sensid41 = "-", "-", "-", "-", "-"
local sensid42, sensid43, sensid44, sensid45, sensid46 = "-", "-", "-", "-", "-"
local sensid47, sensid48, sensid49, sensid50, sensid55 = "-", "-", "-", "-", "-"
local sensid60, sensid61, sensid62, sensid63, sensid64 = "-", "-", "-", "-", "-"
local sensid70, sensid71, sensid72, sensid73, sensid74 = "-", "-", "-", "-", "-"
local sensid75, sensid76, sensid77, sensid78, sensid79 = "-", "-", "-", "-", "-"
local sensid80, sensid81, sensid82, sensid83, sensid84 = "-", "-", "-", "-", "-"
local sensid90, sensid91, sensid92, sensid93 = "-", "-", "-", "-"
local sensid94, sensid95, sensid96, sensid97 = "-", "-", "-", "-"
local rpt = 0
local time, lastTime, newTime = 0, 0, 0
local tSet, anTime, tInit1 = 0, 0, 0
--------------------------------------------------------------------------------
-- Read translations
local function setLanguage()
    local lng=system.getLocale()
    local file = io.readall("Apps/Lang/XSensor3.jsn")
    local obj = json.decode(file)
    if(obj) then
        trans = obj[lng] or obj[obj.default]
    end
end
--------------------------------------------------------------------------------
--ab hier Telemetrieseite 1
--------------------------------------------------------------------------------
-- Draw Fuelgauge and percentage display
local function drawFuelGauge(Value, ox, oy)
		-- Fuelstation
			lcd.drawRectangle(50+ox, 43+oy, 10, 18)
			lcd.drawRectangle(51+ox, 44+oy, 8, 5)
			lcd.drawRectangle(51+ox, 49+oy, 8, 11)
			lcd.drawFilledRectangle(48+ox, 61+oy, 14, 2)
			lcd.drawLine(59+ox, 51+oy, 61+ox, 51+oy)
			lcd.drawLine(62+ox, 52+oy, 62+ox, 56+oy)
			lcd.drawLine(63+ox, 57+oy, 65+ox, 57+oy)
			lcd.drawLine(66+ox, 56+oy, 66+ox, 46+oy)
			lcd.drawLine(66+ox, 46+oy, 63+ox, 43+oy)
			lcd.drawLine(64+ox, 46+oy, 62+ox, 49+oy)
			lcd.drawLine(64+ox, 49+oy, 66+ox, 51+oy)
			
		-- fuel bar 
			lcd.drawText(50+ox, 0+oy, trans.full, FONT_BOLD)
			lcd.drawRectangle (1+ox,81+oy,40,19)	-- lowest bar segment
			lcd.drawRectangle (1+ox,61+oy,40,19)  
			lcd.drawRectangle (1+ox,41+oy,40,19)  
			lcd.drawRectangle (1+ox,21+oy,40,19)  
			lcd.drawRectangle (1+ox,1+oy,40,19)   -- uppermost bar segment
			lcd.drawText(50+ox, 81+oy, trans.empty, FONT_BOLD)

		-- calc bar chart values
			local nSolidBar = math.floor( Value / 20 )
			local nFracBar = (Value - nSolidBar * 20) / 20  -- 0.0 ... 1.0 for fractional bar
			local i

		-- solid bars
			for i=0, nSolidBar - 1, 1 do 
			lcd.drawFilledRectangle (1+ox,81-i*20+oy,40,19) 
			end  

		--  fractional bar
			local y = math.floor( 81-nSolidBar*20+(1-nFracBar)*19 + 0.5)
			lcd.drawFilledRectangle (1+ox,y+oy,40,19*nFracBar) 
					
    collectgarbage()
end
--------------------------------------------------------------------------------
-- Draw Mid top box
local function drawMitopbox(Value, val2, val3)	-- EGT
		-- draw fixed Text
		lcd.drawText(160 - (lcd.getTextWidth(FONT_MINI, trans.fuelvol))/2, 2, trans.fuelvol, FONT_MINI)
		lcd.drawText(190, 34, "ml", FONT_MINI)
		-- draw Values
		lcd.drawText(188 - lcd.getTextWidth(FONT_MAXI, Value),13, Value, FONT_MAXI)		
end
--------------------------------------------------------------------------------
-- Draw left top box
local function drawLetopbox(Value, val2, val3)	-- Drehzahl
		-- draw fixed Text
		lcd.drawText(57 - (lcd.getTextWidth(FONT_MINI,trans.engspeed) / 2),2,trans.engspeed,FONT_MINI)
		lcd.drawText(79, 34, "min", FONT_MINI)
		lcd.drawText(98, 28, "-1", FONT_MINI)

		-- draw Values
		lcd.drawText(77 - lcd.getTextWidth(FONT_MAXI, Value), 13, Value, FONT_MAXI)
		
end
--------------------------------------------------------------------------------
-- Draw left middle box
local function drawLemidbox(Value, val2, val3, val4)	
		-- draw fixed Text
			lcd.drawText(62 - (lcd.getTextWidth(FONT_MINI, trans.temp)/2), 55, trans.temp, FONT_MINI)
			lcd.drawText(37 - (lcd.getTextWidth(FONT_MINI, trans.cyl1)/2), 68, trans.cyl1, FONT_MINI)
			lcd.drawText(44, 86, "°C", FONT_MINI)
			lcd.drawText(85 - (lcd.getTextWidth(FONT_MINI, trans.cyl2)/2), 68, trans.cyl2, FONT_MINI)
			lcd.drawText(93, 86, "°C", FONT_MINI)

			lcd.drawText(4,102,trans.RunTime,FONT_MINI)
			lcd.drawText(112, 102, "h", FONT_MINI)
			lcd.drawText(4,115,trans.totCons,FONT_MINI)
			lcd.drawText(112, 115, "L", FONT_MINI)
		
		-- draw Values
			lcd.drawText(110 - lcd.getTextWidth(FONT_MINI, Value),101, Value, FONT_MINI)
			lcd.drawText(110 - lcd.getTextWidth(FONT_MINI, val2),115, val2, FONT_MINI)
			lcd.drawText(43 - lcd.getTextWidth(FONT_BIG, val3),80, val3, FONT_BIG)
			lcd.drawText(92 - lcd.getTextWidth(FONT_BIG, val4),80, val4, FONT_BIG)
		
end
--------------------------------------------------------------------------------
-- Draw left bottom box
local function drawLebotbox(Value1, Value2, Value3, Value4)	-- Rx Values
		lcd.drawText(15,134,"Rx1: "..Value1.."V  Q:"..Value3, FONT_MINI)
		lcd.drawText(15,146,"Rx2: "..Value2.."V  Q:"..Value4, FONT_MINI)
end
--------------------------------------------------------------------------------
-- Draw right top box
local function drawRitopbox(Value, val2, val3)	-- Altitude
		-- draw fixed Text
		lcd.drawText(262 - (lcd.getTextWidth(FONT_MINI,trans.alt) / 2),2,trans.alt,FONT_MINI)
		lcd.drawText(297, 33, "m", FONT_MINI)

		-- draw Values
		lcd.drawText(295 - lcd.getTextWidth(FONT_MAXI, Value),13, Value, FONT_MAXI)
 		
end
--------------------------------------------------------------------------------
-- Draw right middle box
local function drawRimidbox(Value, val2, val3, val4)	
		-- draw fixed Text
		lcd.drawText(258 - (lcd.getTextWidth(FONT_MINI, trans.speed)/2), 55, trans.speed, FONT_MINI)
		lcd.drawText(279, 85, "km/h", FONT_MINI)
		lcd.drawText(220,102,trans.max,FONT_MINI)
		lcd.drawText(279, 102, "km/h", FONT_MINI)
		lcd.drawText(200,115,trans.totdis,FONT_MINI)
		lcd.drawText(300, 115, "km", FONT_MINI)

		
		-- draw Values
		lcd.drawText(277 - (lcd.getTextWidth(FONT_MAXI, Value)), 65, Value, FONT_MAXI)
		lcd.drawText(277 - (lcd.getTextWidth(FONT_MINI, val3)),102, val3, FONT_MINI)
		lcd.drawText(299 - (lcd.getTextWidth(FONT_MINI, val4)),115, val4, FONT_MINI)
		
end
--------------------------------------------------------------------------------
-- Draw right bottom box
local function drawRibotbox(Value1, Value2, Value3, Value4)	-- Rx Values of Antennas
		-- Empfänger Antennenwerte								
		lcd.drawText(215,135,"Rx1: A1: "..Value1.." A2: "..Value2, FONT_MINI)
		lcd.drawText(215,147,"Rx2: A1: "..Value3.." A2: "..Value4, FONT_MINI)
end
--------------------------------------------------------------------------------
-- Telemetry Page1
local function Page1(width, height)

		-- animation demo at startup - fill tank up
		if( initAnimation ) then
			sensid11 = sensid11 - 5
			if sensid11 <= 0  then
				initAnimation = false
			end
		end	

		drawFuelGauge(sensid11, 130, 57)
		drawMitopbox(sensid10, 0, 0)
		drawLetopbox(sensid1, 0, 0)
		drawLemidbox(sensid3, sensid14, sensid20, sensid21)
		drawLebotbox(rx1Vtg, rx2Vtg, rx1Q, rx2Q)
		drawRitopbox(sensid30, 0, 0)
		drawRimidbox(sensid44, 0, sensid45, sensid50)	
		drawRibotbox(rx1A1, rx1A2, rx2A1, rx2A2)

		-- draw horizontal lines
		lcd.drawFilledRectangle(4, 50, 116, 2)
		lcd.drawFilledRectangle(4, 130, 116, 2)
		lcd.drawFilledRectangle(200, 50, 116, 2)
		lcd.drawFilledRectangle(200, 130, 116, 2)
		
		-- draw vertical lines
		lcd.drawFilledRectangle(109, 4, 2, 42)
		lcd.drawFilledRectangle(210, 4, 2, 42)		

end
-------------------------------------------------------------------------------
-- ab hier Telemetrieseite 2
-------------------------------------------------------------------------------
local function drawBox1(ox, oy, value1, value2, value3, value4, value5)
		-- draw Box
		lcd.drawRectangle(0+ox,0+oy,102, 156, 4)
		lcd.drawText(51+ox - (lcd.getTextWidth(FONT_MINI, "MUI-Sens 1") / 2), 1+oy,"MUI-Sens 1", FONT_MINI)

		-- draw fixed Text
		lcd.drawText(2+ox, 13+oy, "Strom:", FONT_MINI)
		lcd.drawText(73+ox, 32+oy, "A", FONT_MINI)
		
		lcd.drawText(2+ox, 40+oy, "Spannung:", FONT_MINI)
		lcd.drawText(73+ox, 59+oy, "V", FONT_MINI)
		
		lcd.drawText(2+ox, 67+oy, "Leistung:", FONT_MINI)
		lcd.drawText(73+ox, 86+oy, "W", FONT_MINI)
		
		lcd.drawText(2+ox, 94+oy, "Kapazität:", FONT_MINI)
		lcd.drawText(73+ox, 113+oy, "mAh", FONT_MINI)
		
		lcd.drawText(2+ox, 121+oy, "Verbrauch:", FONT_MINI)
		lcd.drawText(73+ox, 140+oy, "mAh", FONT_MINI)
		
		-- draw Values
		lcd.drawText(71+ox - (lcd.getTextWidth(FONT_BIG, value1)), 23+oy, value1, FONT_BIG)
		lcd.drawText(71+ox - (lcd.getTextWidth(FONT_BIG, value2)), 50+oy, value2, FONT_BIG)
		lcd.drawText(71+ox - (lcd.getTextWidth(FONT_BIG, value3)), 77+oy, value3, FONT_BIG)
		lcd.drawText(71+ox - (lcd.getTextWidth(FONT_BIG, value4)), 104+oy, value4, FONT_BIG)
		lcd.drawText(71+ox - (lcd.getTextWidth(FONT_BIG, value5)), 131+oy, value5, FONT_BIG)
		
end
-------------------------------------------------------------------------------
local function drawBox2(ox, oy, value1, value2, value3, value4, value5)
		-- draw Box
		lcd.drawRectangle(0+ox,0+oy,102, 156, 4)
		lcd.drawText(51+ox - (lcd.getTextWidth(FONT_MINI, "MUI-Sens 2") / 2), 1+oy,"MUI-Sens 2", FONT_MINI)

		-- draw fixed Text
		lcd.drawText(4+ox, 15+oy, "Strom:", FONT_MINI)
		lcd.drawText(75+ox, 34+oy, "A", FONT_MINI)
		
		lcd.drawText(4+ox, 42+oy, "Spannung:", FONT_MINI)
		lcd.drawText(75+ox, 61+oy, "V", FONT_MINI)
		
		lcd.drawText(4+ox, 69+oy, "Leistung:", FONT_MINI)
		lcd.drawText(75+ox, 88+oy, "W", FONT_MINI)
		
		lcd.drawText(4+ox, 96+oy, "Kapazität:", FONT_MINI)
		lcd.drawText(75+ox, 115+oy, "mAh", FONT_MINI)
		
		lcd.drawText(4+ox, 123+oy, "Verbrauch:", FONT_MINI)
		lcd.drawText(75+ox, 142+oy, "mAh", FONT_MINI)
		
		-- draw Values
		lcd.drawText(73+ox - (lcd.getTextWidth(FONT_BIG, value1)), 25+oy, value1, FONT_BIG)
		lcd.drawText(73+ox - (lcd.getTextWidth(FONT_BIG, value2)), 52+oy, value2, FONT_BIG)
		lcd.drawText(73+ox - (lcd.getTextWidth(FONT_BIG, value3)), 79+oy, value3, FONT_BIG)
		lcd.drawText(73+ox - (lcd.getTextWidth(FONT_BIG, value4)), 106+oy, value4, FONT_BIG)
		lcd.drawText(73+ox - (lcd.getTextWidth(FONT_BIG, value5)), 133+oy, value5, FONT_BIG)

end
-------------------------------------------------------------------------------
local function drawBox3(ox, oy, value1, value2, value3, value4, value5)
		-- draw Box
		lcd.drawRectangle(0+ox,0+oy,102, 156, 4)
		lcd.drawText(51+ox - (lcd.getTextWidth(FONT_MINI, "MUI-Sens 3") / 2), 1+oy,"MUI-Sens 3", FONT_MINI)

		-- draw fixed Text
		lcd.drawText(4+ox, 15+oy, "Strom:", FONT_MINI)
		lcd.drawText(75+ox, 34+oy, "A", FONT_MINI)
		
		lcd.drawText(4+ox, 42+oy, "Spannung:", FONT_MINI)
		lcd.drawText(75+ox, 61+oy, "V", FONT_MINI)
		
		lcd.drawText(4+ox, 69+oy, "Leistung:", FONT_MINI)
		lcd.drawText(75+ox, 88+oy, "W", FONT_MINI)
		
		lcd.drawText(4+ox, 96+oy, "Kapazität:", FONT_MINI)
		lcd.drawText(75+ox, 115+oy, "mAh", FONT_MINI)
		
		lcd.drawText(4+ox, 123+oy, "Verbrauch:", FONT_MINI)
		lcd.drawText(75+ox, 142+oy, "mAh", FONT_MINI)
		
		-- draw Values
		lcd.drawText(73+ox - (lcd.getTextWidth(FONT_BIG, value1)), 25+oy, value1, FONT_BIG)
		lcd.drawText(73+ox - (lcd.getTextWidth(FONT_BIG, value2)), 52+oy, value2, FONT_BIG)
		lcd.drawText(73+ox - (lcd.getTextWidth(FONT_BIG, value3)), 79+oy, value3, FONT_BIG)
		lcd.drawText(73+ox - (lcd.getTextWidth(FONT_BIG, value4)), 106+oy, value4, FONT_BIG)
		lcd.drawText(73+ox - (lcd.getTextWidth(FONT_BIG, value5)), 133+oy, value5, FONT_BIG)

end
-------------------------------------------------------------------------------
-- Telemetry Page2
local function Page2(width, height)

		-- draw Values
		drawBox1(2, 2, sensid70, sensid71, sensid72, sensid73, sensid74)
--		drawBox2(108, 2, sensid75, sensid76, sensid77, sensid78, sensid79)
--		drawBox3(214, 2, sensid80, sensid81, sensid82, sensid83, sensid84)
		
end
--------------------------------------------------------------------------------
-- Take care of user's settings-changes
local function sensorChanged(value)
	sensorId  = sensorsAvailable[value].id
	system.pSave("sensorId", sensorId)
end

local function alarmValueChanged(value)
    alarmValue = value
    system.pSave("alarmValue", alarmValue)
end

local function alarmVoiceChanged(value)
    alarmVoice = value
    system.pSave("alarmVoice", alarmVoice)
end

local function rptChanged(value)
    rpt = value
    system.pSave("rpt", rpt)
end

local function anSwChanged(value)
    anSw = value
    system.pSave("anSw", anSw)
end

--------------------------------------------------------------------------------
local function setupForm(formID)
    -- List sensors only if menu is active to preserve memory at runtime 
    -- (measured up to 25% save if menu is not opened)
    sensorsAvailable = {}
  local available = system.getSensors();
  local list={}
  local curIndex=-1
  local descr = ""
  for index,sensor in ipairs(available) do 
    if(sensor.param == 0) then
      list[#list+1] = sensor.label
      sensorsAvailable[#sensorsAvailable+1] = sensor
      if(sensor.id==sensorId ) then
        curIndex=#sensorsAvailable
      end 
    end
    end 
    
    local form, addRow, addLabel = form, form.addRow ,form.addLabel
    local addIntbox, addSelectbox = form.addIntbox, form.addSelectbox
    local addInputbox, addCheckbox = form.addInputbox, form.addCheckbox
    local addAudioFilebox, setButton = form.addAudioFilebox, form.setButton
	local setTitle, addSpacer = form.setTitle, form.addSpacer
    
	setTitle(trans.title)
				
	addSpacer(318,7)		

    addRow(1)
    addLabel({label=trans.label1,font=FONT_BOLD})
    
    addRow(2)
    addLabel({label = trans.selectsens, width=200})
    addSelectbox(list, curIndex, true, sensorChanged)
    
	addSpacer(318,7)

    addRow(1)
    addLabel({label=trans.label2,font=FONT_BOLD})
    
	addRow(2)
	addLabel({label=trans.anSw})
	addInputbox(anSw,true,anSwChanged)
	
    addRow(2)
    addLabel({label=trans.rpt, width=230})
    addIntbox(rpt, 0, 1000, 0, 0, 10, rptChanged)
    
    addRow(2)
    addLabel({label=trans.alarmValue, width=230})
    addIntbox(alarmValue, 0, 1500, 0, 0, 10, alarmValueChanged)
    
    addRow(2)
    addLabel({label=trans.voiceFile})
    addAudioFilebox(alarmVoice, alarmVoiceChanged)
	
	addSpacer(318,7)		

	addRow(1)
	addLabel({label="Powered by A. Fromm V"..Version.." ", font=FONT_MINI, alignRight=true})
    
    collectgarbage()
end
---------------------------------------------------------------------------------
local function loop()
		--Read TX Telemetrie
		txTel = system.getTxTelemetry()

			rx1Vtg = string.format("%.2f", txTel.rx1Voltage)
			rx2Vtg = string.format("%.2f", txTel.rx2Voltage)
			rx1Q = string.format("%.0f", txTel.rx1Percent)
			rx2Q = string.format("%.0f", txTel.rx2Percent)
			rx1A1 = string.format("%.0f", txTel.RSSI[1])
			rx1A2 = string.format("%.0f", txTel.RSSI[2])
			rx2A1 = string.format("%.0f", txTel.RSSI[3])
			rx2A2 = string.format("%.0f", txTel.RSSI[4])

		local P8 = ((system.getInputs("P8") + 1) / 2) * 100	--Nur zum Testen!!!
		local anGo = system.getInputsVal(anSw)
		local tTime = system.getTime()
		----------------------------------------------------------------
		-- Read Sensor ID 1 Drehzahl
		sensor = system.getSensorByID(sensorId, 1)
		if(sensor and sensor.valid) then
			sensid1 = string.format("%.0f",sensor.value)
		else
			sensid1 = "-"
		end

		-- Read Sensor ID 2 Umdrehungen insgesamt (Mil)
		sensor = system.getSensorByID(sensorId, 2)
		if(sensor and sensor.valid) then
			sensid2 = string.format("%.2f",sensor.value)
		else
			sensid2 = "-"
		end

		-- Read Sensor ID 3 Betriebszeit (Std.)
		sensor = system.getSensorByID(sensorId, 3)
		if(sensor and sensor.valid) then
			sensid3 = string.format("%.2f",sensor.value)
		else
			sensid3 = "-"
		end	
		-----------------------------------------------------------------------
		-- Read Sensor ID 10 Kraftstoff verbleibend (ml)
		sensor = system.getSensorByID(sensorId, 10)
		if(sensor and sensor.valid) then
			sensid10 = string.format("%.0f",sensor.value)

			-- If we have consumed more than allowed sound alarmValue
			if(not playDone and sensor.value <= alarmValue and alarmVoice ~= "...") then
				system.playFile(alarmVoice,AUDIO_QUEUE)
				system.playNumber(sensor.value, 0, "ml", "R.volume")
				playDone = true   
			end
			-- If we are above consumption alarm-level enable alarm
			if(sensor.value > alarmValue) then playDone = false
			end
			if((not playDone2 or (sensor.value < playValue and rpt > 0)) and anGo == 1 and anTime < tTime) then
				system.playNumber(sensor.value, 0, "ml", "R.volume")
				anTime = tTime + 5
				playDone2 = true
				playValue = (sensor.value - rpt) + 2
			end
			if(anGo ~= 1 and playDone2) then
				playDone2 = false
			end
		else
			sensid10 = "-"
		end

		-- Read Sensor ID 12 Kraftstoff verbraucht (ml)
		sensor = system.getSensorByID(sensorId, 12)
		if(sensor and sensor.valid) then
			sensid12 = string.format("%.0f",sensor.value)
		else
			sensid12 = "-"
		end

		-- Kraftstoff Restprozentberechnung (%)
		verbl = system.getSensorByID(sensorId, 10)	--verbleibender Kraftstoff in ml
		verbr = system.getSensorByID(sensorId, 12) --verbrauchter Kraftstoff in ml
		if (verbl and verbl.valid and verbr and verbr.valid) then
			sensid11 = (verbl.value / (verbr.value + verbl.value)) * 100								
		else			
			if ( initAnimation ) then
			else
				sensid11 = 0	--dieser Sensorwert ist im Datenstrom vom XSensor entfernt worden
			end
		end
		
		-- Read Sensor ID 13 Kraftstoffdurchfluss (ml/min)
		sensor = system.getSensorByID(sensorId, 13)
		if(sensor and sensor.valid) then
			sensid13 = string.format("%.0f",sensor.value)
		else
			sensid13 = "-"
		end

		-- Read Sensor ID 14 Gesamtverbrauch (Liter)
		sensor = system.getSensorByID(sensorId, 14)
		if(sensor and sensor.valid) then
			sensid14 = string.format("%.2f",sensor.value)
		else
			sensid14 = "-"
		end
		-----------------------------------------------------------------------
		-- Read Sensor ID 20 Temperatur 1 (°C)
		sensor = system.getSensorByID(sensorId, 20)
		if(sensor and sensor.valid) then
			sensid20 = string.format("%.0f",sensor.value)
		else
			sensid20 = "-"
		end

		-- Read Sensor ID 21 Temperatur 2 (°C)
		sensor = system.getSensorByID(sensorId, 21)
		if(sensor and sensor.valid) then
			sensid21 = string.format("%.0f",sensor.value)
		else
			sensid21 = "-"
		end

		-- Read Sensor ID 22 Temperatur 3 (°C)
		sensor = system.getSensorByID(sensorId, 21)
		if(sensor and sensor.valid) then
			sensid22 = string.format("%.0f",sensor.value)
		else
			sensid22 = "-"
		end

		-- Read Sensor ID 23 Temperatur 4 (°C)
		sensor = system.getSensorByID(sensorId, 23)
		if(sensor and sensor.valid) then
			sensid23 = string.format("%.0f",sensor.value)
		else
			sensid23 = "-"
		end

		-- Read Sensor ID 24 Temperatur 5 (°C)
		sensor = system.getSensorByID(sensorId, 24)
		if(sensor and sensor.valid) then
			sensid24 = string.format("%.0f",sensor.value)
		else
			sensid24 = "-"
		end
		
		-- Read Sensor ID 25 Temperatur 6 (°C)
		sensor = system.getSensorByID(sensorId, 25)
		if(sensor and sensor.valid) then
			sensid25 = string.format("%.0f",sensor.value)
		else
			sensid25 = "-"
		end

		-- Read Sensor ID 26 Temperatur 7 (°C)
		sensor = system.getSensorByID(sensorId, 26)
		if(sensor and sensor.valid) then
			sensid26 = string.format("%.0f",sensor.value)
		else
			sensid26 = "-"
		end
		-----------------------------------------------------------------------
		-- Read Sensor ID 30 relative Höhe (m)
		sensor = system.getSensorByID(sensorId, 30)
		if(sensor and sensor.valid) then
			sensid30 = string.format("%.0f",sensor.value)
		else
			sensid30 = "-"
		end

		-- Read Sensor ID 31 zurückgelegte Höhe gesamt (km)
		sensor = system.getSensorByID(sensorId, 31)
		if(sensor and sensor.valid) then
			sensid31 = string.format("%.2f",sensor.value)
		else
			sensid31 = "-"
		end

		-- Read Sensor ID 35 Variometer (m/s)
		sensor = system.getSensorByID(sensorId, 35)
		if(sensor and sensor.valid) then
			sensid35 = string.format("%.2f",sensor.value)
		else
			sensid35 = "-"
		end
		-----------------------------------------------------------------------
		-- Read Sensor ID 40 Anzahl empfangener Sateliten (#)
		sensor = system.getSensorByID(sensorId, 40)
		if(sensor and sensor.valid) then
			sensid40 = string.format("%.0f",sensor.value)
		else
			sensid40 = "-"
		end

		-- Read Sensor ID 41 Latitude (°)
		sensor = system.getSensorByID(sensorId, 41)
		if(sensor and sensor.valid) then
			sensid41 = string.format("%.0f",sensor.valGPS)
		else
			sensid41 = "-"
		end

		-- Read Sensor ID 42 Longitude (°)
		sensor = system.getSensorByID(sensorId, 42)
		if(sensor and sensor.valid) then
			sensid42 = string.format("%.0f",sensor.valGPS)
		else
			sensid42 = "-"
		end

		-- Read Sensor ID 43 Höhe (m)
		sensor = system.getSensorByID(sensorId, 43)
		if(sensor and sensor.valid) then
			sensid43 = string.format("%.0f",sensor.value)			
		else
			sensid43 = "-"
		end

		-- Read Sensor ID 44 Geschwindigkeit (km/h)
		sensor = system.getSensorByID(sensorId, 44)
		if(sensor and sensor.valid) then
			sensid44 = string.format("%.0f",sensor.value)			
		else
			sensid44 = "-"
		end

		-- Read Sensor ID 45 Geschwindigkeit max. (km/h)
		sensor = system.getSensorByID(sensorId, 45)
		if(sensor and sensor.valid) then
			sensid45 = string.format("%.0f",sensor.value)			
		else
			sensid45 = "-"
		end

		-- Read Sensor ID 46 Heading (°)
		sensor = system.getSensorByID(sensorId, 46)
		if(sensor and sensor.valid) then
			sensid46 = string.format("%.0f",sensor.value)			
		else
			sensid46 = "-"
		end

		-- Read Sensor ID 47 Kurs (°)
		sensor = system.getSensorByID(sensorId, 47)
		if(sensor and sensor.valid) then
			sensid47 = string.format("%.0f",sensor.value)			
		else
			sensid47 = "-"
		end

		-- Read Sensor ID 48 Distanz zu (m)
		sensor = system.getSensorByID(sensorId, 48)
		if(sensor and sensor.valid) then
			sensid48 = string.format("%.0f",sensor.value)			
		else
			sensid48 = "-"
		end

		-- Read Sensor ID 49 Einzelstrecke (m)
		sensor = system.getSensorByID(sensorId, 49)
		if(sensor and sensor.valid) then
			sensid49 = string.format("%.0f",sensor.value)			
		else
			sensid49 = "-"
		end

		-- Read Sensor ID 50 Zurückgelegte Strecke gesamt (km)
		sensor = system.getSensorByID(sensorId, 50)
		if(sensor and sensor.valid) then
			sensid50 = string.format("%.2f",sensor.value)			
		else
			sensid50 = "-"
		end
		-----------------------------------------------------------------------
		-- Read Sensor ID 55 Geschwindigkeit Staurohr
		sensor = system.getSensorByID(sensorId, 55)
		if(sensor and sensor.valid) then
			sensid55 = string.format("%.0f",sensor.value)			
		else
			sensid55 = "-"
		end
		-----------------------------------------------------------------------
		-- Read Sensor ID 60 Empfängerspannung (V)
		sensor = system.getSensorByID(sensorId, 60)
		if(sensor and sensor.valid) then
			sensid60 = string.format("%.2f",sensor.value)
		else
			sensid60 = "-"
		end
--[[		-----------------------------------------------------------------------
		-- Read Sensor ID 61 Spannung 1 (V)
		sensor = system.getSensorByID(sensorId, 61)
		if(sensor and sensor.valid) then
			sensid61 = string.format("%.2f",sensor.value)
		else
			sensid61 = "-"
		end
		-----------------------------------------------------------------------
		-- Read Sensor ID 62 Spannung 2 (V)
		sensor = system.getSensorByID(sensorId, 62)
		if(sensor and sensor.valid) then
			sensid62 = string.format("%.2f",sensor.value)
		else
			sensid62 = "-"
		end
		-----------------------------------------------------------------------
		-- Read Sensor ID 63 Spannung 3 (V)
		sensor = system.getSensorByID(sensorId, 63)
		if(sensor and sensor.valid) then
			sensid63 = string.format("%.2f",sensor.value)
		else
			sensid63 = "-"
		end
		-----------------------------------------------------------------------
		-- Read Sensor ID 64 Spannung 4 (V)
		sensor = system.getSensorByID(sensorId, 64)
		if(sensor and sensor.valid) then
			sensid64 = string.format("%.2f",sensor.value)
		else
			sensid64 = "-"
		end
--[[		-----------------------------------------------------------------------
		-- Read Sensor ID 70 MUI 1 Strom (A)
		sensor = system.getSensorByID(sensorId, 70)
		if(sensor and sensor.valid) then
			if (sensor.value < 100.0) then
				sensid70 = string.format("%.1f",sensor.value)
			else
				sensid70 = string.format("%.0f",sensor.value)
			end
		else
			sensid70 = "-"
		end

		-- Read Sensor ID 71 MUI 1 Spannung (V)
		sensor = system.getSensorByID(sensorId, 71)
		if(sensor and sensor.valid) then
			sensid71 = string.format("%.2f",sensor.value)
		else
			sensid71 = "-"
		end

		-- Read Sensor ID 72 MUI 1 Leistung (W)
		sensor = system.getSensorByID(sensorId, 72)
		if(sensor and sensor.valid) then
			sensid72 = string.format("%.0f",sensor.value)
		else
			sensid72 = "-"
		end

		-- Read Sensor ID 73 MUI 1 entnommene Kapazität (mAh)
		sensor = system.getSensorByID(sensorId, 73)
		if(sensor and sensor.valid) then
			sensid73 = string.format("%.0f",sensor.value)
		else
			sensid73 = "-"
		end

		-- Read Sensor ID 74 MUI 1 Kapazität (mAh)
		sensor = system.getSensorByID(sensorId, 73)
		if(sensor and sensor.valid) then
			sensid74 = string.format("%.0f",sensor.value)
		else
			sensid74 = "-"
		end
		-----------------------------------------------------------------------
		-- Read Sensor ID 75 MUI 2 Strom (A)
		sensor = system.getSensorByID(sensorId, 75)
		if(sensor and sensor.valid) then
			if (sensor.value < 100.0) then
				sensid75 = string.format("%.1f",sensor.value)
			else
				sensid75 = string.format("%.0f",sensor.value)
			end
		else
			sensid75 = "-"
		end

		-- Read Sensor ID 76 MUI 2 Spannung (V)
		sensor = system.getSensorByID(sensorId, 76)
		if(sensor and sensor.valid) then
			sensid76 = string.format("%.2f",sensor.value)
		else
			sensid76 = "-"
		end

		-- Read Sensor ID 77 MUI 2 Leistung (W)
		sensor = system.getSensorByID(sensorId, 77)
		if(sensor and sensor.valid) then
			sensid77 = string.format("%.0f",sensor.value)
		else
			sensid77 = "-"
		end

		-- Read Sensor ID 78 MUI 2 Kapazität entnommen (mAh)
		sensor = system.getSensorByID(sensorId, 78)
		if(sensor and sensor.valid) then
			sensid78 = string.format("%.0f",sensor.value)
		else
			sensid78 = "-"
		end

		-- Read Sensor ID 79 MUI 2 Kapazität (mAh)
		sensor = system.getSensorByID(sensorId, 79)
		if(sensor and sensor.valid) then
			sensid79 = string.format("%.0f",sensor.value)
		else
			sensid79 = "-"
		end
		-----------------------------------------------------------------------
		-- Read Sensor ID 80 MUI 3 Strom (A)
		sensor = system.getSensorByID(sensorId, 80)
		if(sensor and sensor.valid) then
			if (sensor.value < 100.0) then
				sensid80 = string.format("%.1f",sensor.value)
			else
				sensid80 = string.format("%.0f",sensor.value)
			end
		else
			sensid80 = "-"
		end

		-- Read Sensor ID 81 MUI 3 Spannung (V)
		sensor = system.getSensorByID(sensorId, 81)
		if(sensor and sensor.valid) then
			sensid81 = string.format("%.2f",sensor.value)
		else
			sensid81 = "-"
		end

		-- Read Sensor ID 82 MUI 3 Leistung (W)
		sensor = system.getSensorByID(sensorId, 82)
		if(sensor and sensor.valid) then
			sensid82 = string.format("%.0f",sensor.value)
		else
			sensid82 = "-"
		end

		-- Read Sensor ID 83 MUI 3 Kapazität entnommen (mAh)
		sensor = system.getSensorByID(sensorId, 83)
		if(sensor and sensor.valid) then
			sensid83 = string.format("%.0f",sensor.value)
		else
			sensid83 = "-"
		end

		-- Read Sensor ID 84 MUI 3 Kapazität (mAh)
		sensor = system.getSensorByID(sensorId, 84)
		if(sensor and sensor.valid) then
			sensid84 = string.format("%.0f",sensor.value)
		else
			sensid84 = "-"
		end
--]]		-----------------------------------------------------------------------
		-- Read Sensor ID 90 G-Kraft X (G)
		sensor = system.getSensorByID(sensorId, 90)
		if(sensor and sensor.valid) then
			sensid90 = string.format("%.0f",sensor.value)
		else
			sensid90 = "-"
		end
		
		-- Read Sensor ID 91 G-Kraft Y (G)
		sensor = system.getSensorByID(sensorId, 91)
		if(sensor and sensor.valid) then
			sensid91 = string.format("%.0f",sensor.value)
		else
			sensid91 = "-"
		end
		
		-- Read Sensor ID 92 G-Kraft Z (G)
		sensor = system.getSensorByID(sensorId, 92)
		if(sensor and sensor.valid) then
			sensid92 = string.format("%.0f",sensor.value)
		else
			sensid92 = "-"
		end
		
		-- Read Sensor ID 95 G-Kraft max X (G)
		sensor = system.getSensorByID(sensorId, 95)
		if(sensor and sensor.valid) then
			sensid95 = string.format("%.0f",sensor.value)
		else
			sensid95 = "-"
		end
		
		-- Read Sensor ID 96 G-Kraft max Y (G)
		sensor = system.getSensorByID(sensorId, 96)
		if(sensor and sensor.valid) then
			sensid96 = string.format("%.0f",sensor.value)
		else
			sensid96 = "-"
		end
		
		-- Read Sensor ID 97 G-Kraft max Z (G)
		sensor = system.getSensorByID(sensorId, 97)
		if(sensor and sensor.valid) then
			sensid97 = string.format("%.0f",sensor.value)
		else
			sensid97 = "-"
		end
		
   collectgarbage()
   
end
--------------------------------------------------------------------------------
local function init(code1)
    local pLoad, registerForm, registerTelemetry = system.pLoad, system.registerForm, system.registerTelemetry
	remaining = 100
	model = system.getProperty("Model")
	owner = system.getUserName()
	today = system.getDateTime()
	sensorId = pLoad("sensorId", 0)
    alarmValue = pLoad("alarmValue", 0)
    alarmValue = pLoad("rpt", 0)
    alarmVoice = pLoad("alarmVoice", "...")
	anSw = pLoad("anSw")
    registerForm(1, MENU_APPS, trans.appName1.." ".."V"..Version, setupForm)
    registerTelemetry(1, trans.appName1.." ".."V"..Version.." "..model, 4, Page1) --registers a full size Window
--    registerTelemetry(2, trans.appName1.."2".." ".."V"..Version.." "..model, 4, Page2) --registers a full size Window
    collectgarbage()
end
--------------------------------------------------------------------------------
setLanguage()
collectgarbage()
return {init=init, loop=loop, author="A. Fromm", version=Version, name=trans.appName1}