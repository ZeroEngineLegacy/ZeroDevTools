#include "Precompiled.hpp"
#include "Engine/EngineContainers.hpp"
#include "Configuration.hpp"
#include "Platform/FileSystem.hpp"
#include "Engine/Documentation.hpp"
#include "Serialization/Simple.hpp"
#include "Serialization/Text.hpp"
#include "Pages.hpp"
#include "Support/FileSupport.hpp"
#include "Platform/FilePath.hpp"

namespace Zero
{

	typedef HashMap<String, Array<ClassDoc*> > DocToTags;
	typedef DocToTags::range DocRange;
	String tab(size_t numTabs, size_t numSpaces = 0);
	const size_t TAB_LENGTH = 4;
	void WriteTagIndices(String outputDir, DocToTags& tagged)
	{
		DocRange r = tagged.all();

		StringBuilder markup;
		markup << "Code Index\n";
		markup << "==========" << "\n\n";


		markup << "Vector Math\n";
		markup << "-----------\n\n";
		markup << "*  :doc:`Reference/Vector`\n";
		markup << "*  :doc:`Reference/Quat`\n\n";

		for (; !r.empty(); r.popFront())
		{
			Array<ClassDoc*>& stuff = r.front().second;
			sort(stuff.all());

			String tag = r.front().first;

			markup << tag << "\n";
			markup << String::Repeat('-', tag.size()) << "\n\n";

			forRange(ClassDoc* doc, stuff.all())
			{
				markup << String::Format("*  :doc:`Reference/%s`\n\n", doc->Name.c_str(), doc->Name.c_str());
			}

			markup << "\n";
		}

		// Write the toctree
		markup << ".. toctree::\n"
			<< tab(1u, TAB_LENGTH) << ":glob:\n"
			<< tab(1u, TAB_LENGTH) << ":hidden:\n"
			<< tab(1u, TAB_LENGTH) << ":maxdepth: 1\n\n"
			<< tab(1u, TAB_LENGTH) << "Reference/*";


		String fileName = FilePath::Combine(outputDir, "..", "index.rst");

		String text = markup.ToString();

		WriteStringRangeToFile(fileName, text);
	}


	void WriteClass(String directory, ClassDoc& classDoc, DocToTags& tagged)
	{
		//printf("Class %s\n", classDoc.Name.c_str());

		forRange(String tag, classDoc.Tags.all())
			tagged[tag].push_back(&classDoc);

		if (classDoc.Tags.empty())
			tagged["Various"].push_back(&classDoc);

		String outputDir = directory;

		StringBuilder classMarkup;

		classMarkup << ".. _Reference" << classDoc.Name << ":" << "\n\n";

		classMarkup << ".. rst-class:: " << "searchtitle\n\n";
		classMarkup << classDoc.Name << "\n";
		classMarkup.Repeat(classDoc.Name.size(), "=");
		classMarkup << "\n\n";

        if (!classDoc.Description.empty())
        {
      	    classMarkup << ".. rst-class:: " << "searchdescription\n\n";
       	    classMarkup << classDoc.Description << "\n\n";
        }
		classMarkup << ".. include:: Description/" << classDoc.Name << ".rst\n\n";

		classMarkup << ".. cpp:type:: " << classDoc.Name << "\n\n";

		if (!classDoc.BaseClass.empty())
			classMarkup << tab(1u, TAB_LENGTH) << "Base Class: " << ":cpp:type:`" << classDoc.BaseClass << "`" << "\n\n";

		classMarkup << ".. _Reference" << classDoc.Name << "Properties:" << "\n\n";

		if (!classDoc.Properties.empty())
		{
			classMarkup << "Properties\n----------\n\n";

			forRange(PropertyDoc& propertyDoc, classDoc.Properties.all())
			{
				if (propertyDoc.Name == "Name" && propertyDoc.Description.empty())
					continue;

				classMarkup << tab(1u, TAB_LENGTH) << ".. rst-class:: " << "collapsible\n\n";
				classMarkup << tab(2u, TAB_LENGTH) << ".. cpp:member:: " << propertyDoc.Type << " " << propertyDoc.Name << "\n\n";
				classMarkup << tab(3u, TAB_LENGTH) << propertyDoc.Description << "\n\n";
			}
		}
		classMarkup << ".. _Reference" << classDoc.Name << "Methods:" << "\n\n";

		if (!classDoc.Methods.empty())
		{
			classMarkup << "Methods\n-------\n\n";

			forRange(MethodDoc& methodDoc, classDoc.Methods.all())
			{
				//classMarkup << ".. collapsible:: " << methodDoc.Name << methodDoc.Arguments << "\n\n";
				classMarkup << tab(1u, TAB_LENGTH) << ".. rst-class:: " << "collapsible\n\n";
				classMarkup << tab(2u, TAB_LENGTH) << ".. cpp:function:: " << methodDoc.ReturnValue << " "
					<< methodDoc.Name << methodDoc.Arguments << "\n\n";

				classMarkup << tab(3u, TAB_LENGTH) << methodDoc.Description << "\n\n";
			}
		}

		classMarkup << ".. include:: Remarks/" << classDoc.Name << ".rst";

		Zero::CreateDirectoryAndParents(outputDir);

		String filename = BuildString(classDoc.Name, ".rst");

		String fullPath = FilePath::Combine(outputDir, filename);

		String text = classMarkup.ToString();

		WriteStringRangeToFile(fullPath, text);

	}

	void WriteOutMarkup(StringMap& params)
	{
		printf("Mark up\n");

		Configurations config = LoadConfigurations(params);

		DocumentationLibrary doc;
		LoadFromDataFile(doc, config.DocumentationFile);
		doc.Build();

		String directory = GetStringValue<String>(params, "output", "Markup");
		DocToTags tagged;

		//Now load the documentation file (the documentation for all the classes)
		if (!FileExists(config.DocumentationFile.c_str()))
		{
			printf("%s does not exist.", config.DocumentationFile.c_str());
			return;
		}

		//Upload the class' page to the wiki, making sure to perform the link replacements
		forRange(ClassDoc& classDoc, doc.Classes.all())
		{
			WriteClass(directory, classDoc, tagged);
		}

		WriteTagIndices(directory, tagged);
	}

	String tab(size_t numTabs, size_t numSpaces)
	{
		StringBuilder whitespace;
		String tab;
		// generate the token representing one tab
		if (numSpaces != 0) // if we are using spaces & not tab characters
		{
			tab = String::Repeat(' ', numSpaces);
		}
		else // if we must use tabs.
		{
			tab = String::Repeat('\t', 1);
		}

		// append as many as requested
		whitespace.Repeat(numTabs, tab);
		return whitespace.ToString();
	}
}

