[WorkbenchToolAttribute("TerrainBuilder objects Import/Export", "Import TerrainBuilder data and create entities", "2", awesomeFontCode: 0xf2f6)]
class TerrainBuilderImportTool: WorldEditorTool
{
	[Attribute("", UIWidgets.ResourceNamePicker, "Terrain Builder objects file", "txt")]
	ResourceName ObjectsFile;

	[Attribute("", UIWidgets.ResourceNamePicker, "A file that stores mappings between Arma Reforger and Terrain Builder", "txt")]
	ResourceName MappingFile;

	[Attribute("true", UIWidgets.CheckBox, "Use realtive height else absolute")]
	bool useRelative;

	[Attribute("200000", UIWidgets.SpinBox, "This is the easting of the map in Terrain Builder")]
	float easting;

	[Attribute("0", UIWidgets.SpinBox, "This is the northing of the map in Terrain Builder")]
	float northing;

	[Attribute(desc: "Map Terrain Builder Template Library names to Enfusion Prefabs")]
	protected ref array<ref TBToEnfusionMapping> TBToEnfusion;

	ref map<string, string> mappings;

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

		mappings = new map<string,string>;
		map<string,string> errors = new map<string,string>;

		foreach(TBToEnfusionMapping m : TBToEnfusion)
		{
			if(mappings.Contains(m.TB_name))
			{
				Print("Mapping for" + m.TB_name + " already exist.", LogLevel.WARNING);
			} else {
				mappings.Insert(m.TB_name, m.AR_Prefab);
			}
		}
		

		if (m_API.BeginEntityAction())
		{
			array<string> tokens = new array<string>();
			for (int i = 0; true; ++i)
			{
				int numTokens = parser.ParseLine(i, tokens);
				if(i % 1000 == 0)
					Print("imported: "+ i);

				if (numTokens == 0)
					break;

				if (numTokens != 16)
				{
					errors.Insert("line"+i,"Line " + i + ": Invalid data format expected 16 tokens got " + numTokens  + ", "+ tokens.ToString());
					continue;
				}
				// 		0 		1		2			3 		4 		 5 		6 		  7 	8 		 9 		10 		11 		12 	  13	14
				// {'1_ruins',';','208209.546875',';','1733.600708',';','307.892517',';','0.000000',';','0.000000',';','1.000000',';','10.198336',';'}
				string resourceHash = mappings.Get(tokens[0]);
			
				if(!resourceHash)
				{
					if(!errors.Contains(tokens[0]))
					{
						errors.Insert(tokens[0],"Could not find TBToEnfusion mapping for " + tokens[0] + " | Resource Path: "  + resourceHash);
					}
					continue;
				}

				vector pos = {tokens[2].ToFloat(),tokens[14].ToFloat(),tokens[4].ToFloat()};
				vector angles = {tokens[8].ToFloat(),tokens[6].ToFloat(),tokens[10].ToFloat()};
				pos[0] = pos[0] - easting;
				pos[2] = pos[2] - northing;
				float scale = tokens[12].ToFloat();

				if (useRelative)
				{
					pos[1] = pos[1] - m_API.GetTerrainSurfaceY(pos[0], pos[2]);
				}

				IEntity ent = m_API.CreateEntity(resourceHash, "", m_API.GetCurrentEntityLayerId(), null, pos, angles);
				if (ent == null)
				{
					Print("Line " + i + ": Entity could not be created", LogLevel.ERROR);
					continue;
				}
				m_API.ModifyEntityKey(ent, "scale", scale.ToString());
			}
			m_API.EndEntityAction();
		}

		parser.EndParse();
		foreach(string k, string v : errors)
		{
			Print(v, LogLevel.ERROR);
		}
		Print("Import done");
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
		
		mappings = new map<string,string>;
		map<string,string> errors = new map<string,string>;

		foreach(TBToEnfusionMapping m : TBToEnfusion)
		{
			if(mappings.Contains(m.TB_name))
			{
				Print("Mapping for" + m.TB_name + " already exist.", LogLevel.WARNING);
			} else {
				mappings.Insert(m.AR_Prefab,m.TB_name);
			}
		}
		
		string format  = "\"%1\";%2;%3;%4;%5;%6;%7;%8;";
		
		EditorEntityIterator iter(m_API);
		int approximateCount = m_API.GetEditorEntityCount();
		
		Print("Exporting entities: " + approximateCount);
		
		WorldEditor we = Workbench.GetModule(WorldEditor);
		WBProgressDialog progress = new WBProgressDialog("Exporting...", we);
		
		IEntitySource src = iter.GetNext();
		while (src)
		{
			IEntity entity = m_API.SourceToEntity(src);
			
			ResourceName res = entity.GetPrefabData().GetPrefab().GetResourceName();
			
			if(!res)
			{
				src = iter.GetNext();
				progress.SetProgress(iter.GetCurrentIdx() / approximateCount);
				continue;
			}
			
			string tbname = mappings.Get(res);
			
			if(res == "" || tbname == "")
			{
				if(!errors.Contains(res))
				{
					errors.Insert(res, "Could not find TBToEnfusion mapping for " + res + " | Resource Path: "  + tbname);
				}
				src = iter.GetNext();
				progress.SetProgress(iter.GetCurrentIdx() / approximateCount);
				continue;
			}
			
			if(entity && tbname) {
				float scale = entity.GetScale();
				vector angles = entity.GetAngles();
				vector mat[4];
				entity.GetTransform(mat);
				vector pos = mat[3];
				pos[0] = pos[0] + easting;
				pos[2] = pos[2] + northing;
				
				if (useRelative)
				{
					pos[1] = pos[1] - m_API.GetTerrainSurfaceY(pos[0], pos[2]);
				}
	
				// 		0 		1		2			3 		4 		 5 		6 		  7 	8 		 9 		10 		11 		12 	  13	14
				// {'1_ruins',';','208209.546875',';','1733.600708',';','307.892517',';','0.000000',';','0.000000',';','1.000000',';','10.198336',';'}
				file.FPrintln(string.Format(format, tbname, pos[0] , pos[2], angles[1], angles[0], angles[2], scale, pos[1]));
				//file.FPrintln(string.Format(format, tbname, pos[0].ToString(-1,6) , pos[2].ToString(-1,6), angles[1].ToString(-1,6), angles[0].ToString(-1,6), angles[2].ToString(-1,6), scale.ToString(-1,6), pos[1].ToString(-1,6)));
			}
			
			src = iter.GetNext();
			progress.SetProgress(iter.GetCurrentIdx() / approximateCount);
		}
		
		foreach(string k, string v : errors)
		{
			Print(v, LogLevel.ERROR);
		}	
		Print("Export done");
		if (file)
		{
			file.CloseFile();
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
		
		Print("Importing Mapping...");
		array<string> tokens = new array<string>();
		for (int i = 0; true; ++i)
		{
			int numTokens = parser.ParseLine(i, tokens);
			if (numTokens == 0)
					break;
			if (numTokens != 2)
			{
				Print("Line " + i + ": Invalid data format expected 2 tokens got " + numTokens, LogLevel.ERROR);
				continue;
			}
			TBToEnfusionMapping m = new TBToEnfusionMapping();
			m.Init(tokens[0], tokens[1]);
			Print(tokens[0]+" : "+ tokens[1]);
			TBToEnfusion.Insert(m);
		}
		Print("Mapping Imported");
	}

	[ButtonAttribute("Export mapping")]
	void ExportMapping()
	{
		Print("Exporting Mapping...");
		string exportPath = MappingFile.GetPath();
		FileHandle file = FileIO.OpenFile(exportPath , FileMode.WRITE);
		if (!file)
		{
			Print("Unable to open file for writting " + exportPath , LogLevel.ERROR);
			return;
		}
		foreach(TBToEnfusionMapping m : TBToEnfusion)
		{
			Print(m.TB_name + " : " + m.AR_Prefab);
			file.FPrintln("\""+m.TB_name + "\" \"" + m.AR_Prefab +"\"");
		}
		if (file)
		{
			file.CloseFile();
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
class TBToEnfusionMapping{
	[Attribute(desc: "Arma Reforger Prefab",params:"et")]
	ResourceName AR_Prefab;

	[Attribute(desc: "Terrain Builder template string name")]
	string TB_name;

	void Init(string name = "", string res = "")
	{
		TB_name = name;
		AR_Prefab = res;
	}
}
