[WorkbenchToolAttribute("TerrainBuilder objects Import/Export", "Import TerrainBuilder data and create entities", "2", awesomeFontCode: 0xf2f6)]
class TerrainBuilderImportTool: WorldEditorTool
{
	[Attribute("", UIWidgets.ResourceNamePicker, "Terrain Builder objects file", "txt")]
	ResourceName ObjectsFile;

	[Attribute("", UIWidgets.ResourceNamePicker, "A file that stores mappings between Arma Reforger and Terrain Builder", "txt")]
	ResourceName MappingFile;

	[Attribute("false", UIWidgets.CheckBox, "Use realtive height else absolute")]
	bool useRelative;

	[Attribute("true", UIWidgets.CheckBox, "Only Export active layer")]
	bool exportOnlyActiveLayer;

	[Attribute("200000", UIWidgets.SpinBox, "This is the easting of the map in Terrain Builder")]
	float easting;

	[Attribute("0", UIWidgets.SpinBox, "This is the northing of the map in Terrain Builder")]
	float northing;

	[Attribute(desc: "Map Terrain Builder Template Library names to Enfusion Prefabs")]
	protected ref array<ref TBToEnfusionMapping> TBToEnfusion;

	ref map<string, TBToEnfusionMapping> mappings;

	[ButtonAttribute("Import objects")]
	void ImportData()
	{
		ParseHandle parser = FileIO.BeginParse(ObjectsFile.GetPath());
		if (parser == 0)
		{
			Print("Could not open file: " + ObjectsFile.GetPath(), LogLevel.ERROR);
			return;
		}
		Print("Importing objects");

		mappings = new map<string,TBToEnfusionMapping>;
		map<string,string> errors = new map<string,string>;

		foreach(TBToEnfusionMapping m : TBToEnfusion)
		{
			if(mappings.Contains(m.TB_name))
			{
				Print("Mapping for" + m.TB_name + " already exist.", LogLevel.WARNING);
			} else {
				mappings.Insert(m.TB_name, m);
			}
		}
		int importedAmount = 0;
		

		if (m_API.BeginEntityAction())
		{
			array<string> tokens = new array<string>();
			for (int i = 0; true; ++i)
			{
				int numTokens = parser.ParseLine(i, tokens);

				if (numTokens == 0)
					break;

				if (numTokens != 16)
				{
					errors.Insert("line"+i,"Line " + i + ": Invalid data format expected 16 tokens got " + numTokens  + ", "+ tokens.ToString());
					continue;
				}
				// 		0 		1		2			3 		4 		 5 		6 		  7 	8 		 9 		10 		11 		12 	  13	14
				// {'1_ruins',';','208209.546875',';','1733.600708',';','307.892517',';','0.000000',';','0.000000',';','1.000000',';','10.198336',';'}
				TBToEnfusionMapping mapp = mappings.Get(tokens[0]);
			
				if(!mapp)
				{
					if(!errors.Contains(tokens[0]))
					{
						errors.Insert(tokens[0],"Could not find TBToEnfusion mapping for " + tokens[0] + " | Resource Path: "  + mapp.AR_Prefab);
					}
					continue;
				}

				vector pos = {tokens[2].ToFloat(),tokens[14].ToFloat(),tokens[4].ToFloat()};
				vector angles = {tokens[8].ToFloat(),tokens[6].ToFloat(),tokens[10].ToFloat()};
				float scale = tokens[12].ToFloat();

				if (useRelative)
				{
					pos[1] = pos[1] + m_API.GetTerrainSurfaceY(pos[0], pos[2]);
				}
				
				pos[0] = pos[0] - easting;
				pos[2] = pos[2] - northing;
				
				pos[0] = pos[0] + mapp.Offset[0];
				pos[1] = pos[1] + mapp.Offset[1];
				pos[2] = pos[2] + mapp.Offset[2];
				angles[1] = angles[1] + mapp.Angel;

				IEntitySource ent = m_API.CreateEntity(mapp.AR_Prefab, "", m_API.GetCurrentEntityLayerId(), null, pos, angles);
				if (ent == null)
				{
					Print("Line " + i + ": Entity could not be created", LogLevel.ERROR);
					continue;
				}
				m_API.ModifyEntityKey(ent, "scale", scale.ToString());
				importedAmount++;
			}
			m_API.EndEntityAction();
		}

		parser.EndParse();
		foreach(string k, string v : errors)
		{
			Print(v, LogLevel.ERROR);
		}
		Print("Imported " + importedAmount + " Entities");
	}
	
	[ButtonAttribute("Export objects")]
	void ExportData()
	{
		string exportPath = ObjectsFile.GetPath();
		FileHandle file = FileIO.OpenFile(exportPath , FileMode.WRITE);
		
		if (!file)
		{
			Print("Unable to open file for writting " + exportPath , LogLevel.ERROR);
			return;
		}
		
		mappings = new map<string,TBToEnfusionMapping>;
		map<string,string> errors = new map<string,string>;

		foreach(TBToEnfusionMapping m : TBToEnfusion)
		{
			if(mappings.Contains(m.TB_name))
			{
				Print("Mapping for" + m.TB_name + " already exist.", LogLevel.WARNING);
			} else {
				mappings.Insert(m.AR_Prefab,m);
			}
		}
		
		string format  = "\"%1\";%2;%3;%4;%5;%6;%7;%8;";
		
		EditorEntityIterator iter(m_API);
		int approximateCount = m_API.GetEditorEntityCount();
		int exportedAmount = 0;
		
		Print("Exporting entities: Total found " + approximateCount);
		
		WorldEditor we = Workbench.GetModule(WorldEditor);
		WBProgressDialog progress = new WBProgressDialog("Exporting...", we);
		
		IEntitySource src = iter.GetNext();
		int activeLayerID = m_API.GetCurrentEntityLayerId();
		while (src)
		{
			if(exportOnlyActiveLayer && src.GetLayerID() != activeLayerID)
			{
				src = iter.GetNext();
				progress.SetProgress(iter.GetCurrentIdx() / approximateCount);
				continue;
			}
			IEntity entity = m_API.SourceToEntity(src);
			
			ResourceName res = entity.GetPrefabData().GetPrefab().GetResourceName();
			
			if(!res)
			{
				src = iter.GetNext();
				progress.SetProgress(iter.GetCurrentIdx() / approximateCount);
				continue;
			}
			
			TBToEnfusionMapping mapp = mappings.Get(res);
			
			if(res == "" || mapp.TB_name == "")
			{
				if(!errors.Contains(res))
				{
					errors.Insert(res, "Could not find TBToEnfusion mapping for " + res + " | Resource Path: "  + mapp.TB_name);
				}
				src = iter.GetNext();
				progress.SetProgress(iter.GetCurrentIdx() / approximateCount);
				continue;
			}
			
			if(entity && mapp.TB_name) {
				float scale = entity.GetScale();
				vector angles = entity.GetAngles();
				vector mat[4];
				entity.GetTransform(mat);
				vector pos = mat[3];
				
				if (useRelative)
				{
					pos[1] = pos[1] - m_API.GetTerrainSurfaceY(pos[0], pos[2]);
				}
				
				pos[0] = pos[0] + easting;
				pos[2] = pos[2] + northing;
				exportedAmount++;
	
				// 		0 		1		2			3 		4 		 5 		6 		  7 	8 		 9 		10 		11 		12 	  13	14
				// {'1_ruins',';','208209.546875',';','1733.600708',';','307.892517',';','0.000000',';','0.000000',';','1.000000',';','10.198336',';'}
				file.WriteLine(string.Format(format, mapp.TB_name, pos[0] , pos[2], angles[1], angles[0], angles[2], scale, pos[1]));
				//file.FPrintln(string.Format(format, tbname, pos[0].ToString(-1,6) , pos[2].ToString(-1,6), angles[1].ToString(-1,6), angles[0].ToString(-1,6), angles[2].ToString(-1,6), scale.ToString(-1,6), pos[1].ToString(-1,6)));
			}
			
			src = iter.GetNext();
			progress.SetProgress(iter.GetCurrentIdx() / approximateCount);
		}
		
		foreach(string k, string v : errors)
		{
			Print(v, LogLevel.ERROR);
		}	
		Print("Exported " + exportedAmount + " Entitys");
		if (file)
		{
			file.Close();
		}
	}
	
	[ButtonAttribute("Import mapping")]
	void ImportMapping()
	{
		ParseHandle parser = FileIO.BeginParse(MappingFile.GetPath());
		if (parser == 0)
		{
			Print("Could not open file: " + MappingFile.GetPath(), LogLevel.ERROR);
			return;
		}
		TBToEnfusion.Clear();
		
		array<string> tokens = new array<string>();
		for (int i = 0; true; ++i)
		{
			int numTokens = parser.ParseLine(i, tokens);
			if (numTokens == 0)
					break;
			if (numTokens != 6)
			{
				Print("Line " + i + ": Invalid data format expected 2 tokens got " + numTokens, LogLevel.ERROR);
				continue;
			}
			TBToEnfusionMapping m = new TBToEnfusionMapping();
			m.Init(tokens);
			TBToEnfusion.Insert(m);
		}
		
		UpdatePropertyPanel();
		Print("Mapping Imported");
	}

	[ButtonAttribute("Export mapping")]
	void ExportMapping()
	{
		string exportPath = MappingFile.GetPath();
		string formatMapping  = "\"%1\" \"%2\" %3 %4 %5 %6 %7";
		FileHandle file = FileIO.OpenFile(exportPath , FileMode.WRITE);
		if (!file)
		{
			Print("Unable to open file for writting " + exportPath , LogLevel.ERROR);
			return;
		}
		foreach(TBToEnfusionMapping m : TBToEnfusion)
		{
			file.WriteLine(string.Format(formatMapping, m.TB_name, m.AR_Prefab, m.Offset[0], m.Offset[1] , m.Offset[2], m.Angel));
		}
		if (file)
		{
			file.Close();
		}
		Print("Mapping Exported");
	}

	[ButtonAttribute("Print mapping")]
	void PrintMapping()
	{
		Print("Printing Mapping...");
		foreach(TBToEnfusionMapping m : TBToEnfusion)
		{
			Print("\""+m.TB_name + "\" : \"" + m.AR_Prefab +"\"");
		}
		Print("Mapping Printed");
	}
}

[BaseContainerProps()]
class TBToEnfusionMapping {
	[Attribute(desc: "Arma Reforger Prefab",params:"et")]
	ResourceName AR_Prefab;

	[Attribute(desc: "Terrain Builder template string name")]
	string TB_name;

	[Attribute(desc: "Offset")]
	vector Offset;

	[Attribute(desc: "Angel Y")]
	int Angel;

	void Init(array<string> tokens)
	{
		TB_name = tokens[0];
		AR_Prefab = tokens[1];
		Offset[0] = tokens[2].ToFloat();
		Offset[1] = tokens[3].ToFloat();
		Offset[2] = tokens[4].ToFloat();
		Angel = tokens[5].ToFloat();
	}
}
